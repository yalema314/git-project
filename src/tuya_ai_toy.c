/**
 * @file tuya_ai_toy.c
 * @brief Tuya AI toy (Wukong) core: init, DP process, config load/save, key/led/network,
 *        wukong agent, idle and low-power timers, and public API (trigger mode, volume, timers).
 */

#include <stdio.h>
#include <string.h>
#include "tuya_ai_toy.h"
#include "tuya_ai_agent.h"
#include "tuya_device_cfg.h"
#include "tuya_ai_toy_led.h"
#include "tuya_ai_toy_key.h"
#include "skill_cloudevent.h"
#include "skill_emotion.h"
#include "skill_music_story.h"
#include "wukong_ai_skills.h"
#include "wukong_audio_input.h"
#include "wukong_audio_player.h"
#include "wukong_audio_aec_vad.h"
#include "wukong_ai_agent.h"
#include "wukong_kws.h"
#include "wukong_ai_mode_free.h"
#include "tuya_ws_db.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_thread.h"
#include "tkl_audio.h"
#include "tuya_ringbuf.h"
#include "tuya_devos_utils.h"
#include "tal_semaphore.h"
#include "tal_queue.h"
#include "tal_system.h"
#include "tal_network.h"
#include "tal_mutex.h"
#include "tkl_thread.h"
#include "tal_sw_timer.h"
#include "tal_workq_service.h"
#include "base_event.h"
#include "tuya_iot_com_api.h"
#include "media_src.h"
#include "tuya_key.h"
#include "tuya_ai_client.h"
#include "tuya_ai_biz.h"
#include "tkl_wakeup.h"
#include "audio_dump.h"
#include "tal_time_service.h"
#include "tuya_ai_battery.h"
#include "tal_sleep.h"
#include "wukong_ai_mode.h"
#include "svc_ai_player.h"
#include "tuya_ai_toy_mcp.h"
#if defined(ENABLE_TUYA_UI) && (ENABLE_TUYA_UI == 1)
#include "tkl_display.h"
#include "tuya_ai_display.h"
#endif
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
#include "tuya_iot_wifi_api.h"
#include "tuya_wifi_status.h"
#endif
#if defined(ENABLE_APP_AI_MONITOR) && (ENABLE_APP_AI_MONITOR == 1)
#include "tuya_ai_monitor.h"
#endif
#if defined(ENABLE_TUYA_CAMERA) && (ENABLE_TUYA_CAMERA == 1)
#include "wukong_video_input.h"
#include "tuya_ai_toy_camera.h"
#if defined(ENABLE_MQTT_P2P) && (ENABLE_MQTT_P2P == 1)
#include "tuya_p2p_app.h"
#endif
#endif
#if defined(ENABLE_AUDIO_ANALYSIS) && (ENABLE_AUDIO_ANALYSIS == 1)
#include "audio_analysis.h"
#endif
#include "tkl_gpio.h"

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define AI_TOY_PARA           "ai_toy_para"
#define LONG_KEY_TIME         400
#define SEQ_KEY_TIME          200
#define TOY_NETCFG_PROMPT_TIMEOUT (10 * 1000)
#define TOY_IDLE_TIMEOUT      (30 * 1000)     
#define TOY_LOWPOWER_TIMEOUT  (30 * 60 * 1000)

#define MAX_INPUT_RINGBUG_SIZE  (128 * 1024)
#define MAX_INPUT_BUF_SIZE      (16 * 1024)

#define AI_AUDIO_SLICE_TIME    80   /* Audio slice per output, 80 ms */
#define TOY_VOLUME_SETUP       20   /* Volume step */
#define NETCFG_LONG_KEY_TIME   5000 /* 5 sec */

#define AI_TOY_ALERT_PLAY_ID   "ai_toy_alert"
#define AI_TOY_DP_SWITCH       102  /* 开机并联网成功 */
#define AI_TOY_DP_WAKEUP       101  /* 唤醒状态 */
#define AI_TOY_DP_IDLE         103  /* 空闲状态 */

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
STATIC TY_AI_TOY_T *s_ai_toy = NULL;
STATIC UINT8_T      s_lang = TY_AI_DEFAULT_LANG;
STATIC INT_T        __s_wakeup_flag = 0;
STATIC BOOL_T       s_mcp_inited = FALSE;
STATIC BOOL_T       s_mcp_init_scheduled = FALSE;
STATIC BOOL_T       s_ai_client_ready = FALSE;
STATIC BOOL_T       s_boot_cloud_connected = FALSE;
STATIC BOOL_T       s_boot_switch_reported = FALSE;
STATIC UINT8_T      s_boot_switch_retry_cnt = 0;
STATIC UINT8_T      s_boot_cloud_retry_cnt = 0;
STATIC BOOL_T       s_network_cfg_prompted = FALSE;
STATIC BOOL_T       s_idle_dp_state = FALSE;
STATIC DELAYED_WORK_HANDLE s_boot_switch_delayed_work = NULL;
STATIC TIMER_ID     s_network_cfg_timer = NULL;

#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
STATIC VOID __on_ai_toy_wf_nw_stat_cb(GW_WIFI_NW_STAT_E nw_stat);
#endif

/* ---------------------------------------------------------------------------
 * Config and report helpers
 * --------------------------------------------------------------------------- */

BOOL_T tuya_ai_toy_boot_report_ready(VOID)
{
    return s_boot_switch_reported;
}

VOID tuya_ai_toy_wakeup_dp_report(BOOL_T enable)
{
    TY_OBJ_DP_S dp = {0};
    OPERATE_RET rt = OPRT_OK;

    dp.dpid = AI_TOY_DP_WAKEUP;
    dp.type = PROP_BOOL;
    dp.value.dp_bool = enable;

    rt = dev_report_dp_json_async_force(NULL, &dp, 1);
    if (OPRT_OK == rt) {
        TAL_PR_NOTICE("report wake_up dp success: %d", enable);
    } else {
        TAL_PR_ERR("report wake_up dp failed: %d, rt=%d", enable, rt);
    }
}

STATIC OPERATE_RET __ai_toy_report_switch(BOOL_T on)
{
    TY_OBJ_DP_S dp = {0};
    OPERATE_RET rt = OPRT_OK;

    dp.dpid = AI_TOY_DP_SWITCH;
    dp.type = PROP_BOOL;
    dp.value.dp_bool = on;

    rt = dev_report_dp_json_async_force(NULL, &dp, 1);
    if (OPRT_OK == rt) {
        TAL_PR_NOTICE("report switch dp success: %d", on);
    } else {
        TAL_PR_ERR("report switch dp failed: %d, rt=%d", on, rt);
    }

    return rt;
}

STATIC OPERATE_RET __ai_toy_report_idle(BOOL_T idle)
{
    TY_OBJ_DP_S dp = {0};
    OPERATE_RET rt = OPRT_OK;

    if (s_idle_dp_state == idle) {
        return OPRT_OK;
    }

    dp.dpid = AI_TOY_DP_IDLE;
    dp.type = PROP_BOOL;
    dp.value.dp_bool = idle;

    rt = dev_report_dp_json_async_force(NULL, &dp, 1);
    if (OPRT_OK == rt) {
        s_idle_dp_state = idle;
        TAL_PR_NOTICE("report idle dp success: %d", idle);
    } else {
        TAL_PR_ERR("report idle dp failed: %d, rt=%d", idle, rt);
    }

    return rt;
}

STATIC VOID __ai_toy_report_boot_switch_if_ready(VOID)
{
    if (FALSE == s_boot_cloud_connected || FALSE == s_ai_client_ready || TRUE == s_boot_switch_reported) {
        return;
    }

    if (OPRT_OK == __ai_toy_report_switch(TRUE)) {
        s_boot_switch_reported = TRUE;
        TAL_PR_NOTICE("ai toy -> boot cloud ready, report dp%d=true", AI_TOY_DP_SWITCH);
    } else {
        TAL_PR_ERR("ai toy -> boot cloud ready, report dp%d=true failed", AI_TOY_DP_SWITCH);
    }
}

STATIC VOID __ai_toy_boot_switch_delayed_work_cb(VOID_T *data)
{
    (VOID)data;
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
    GW_WIFI_NW_STAT_E nw_stat = 0;
#endif

    if (TRUE == s_boot_switch_reported) {
        return;
    }

#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
    if (FALSE == s_boot_cloud_connected) {
        if (OPRT_OK == get_wf_gw_nw_status(&nw_stat)) {
            TAL_PR_NOTICE("ai toy -> delayed check wifi network status: %d", nw_stat);
            if (STAT_CLOUD_CONN == nw_stat) {
                __on_ai_toy_wf_nw_stat_cb(nw_stat);
            }
        } else {
            TAL_PR_ERR("ai toy -> delayed get wifi network status failed");
        }

        if (FALSE == s_boot_cloud_connected) {
            if (s_boot_cloud_retry_cnt >= 10) {
                TAL_PR_WARN("ai toy -> wait cloud connect timeout, stop boot dp%d report", AI_TOY_DP_SWITCH);
                return;
            }

            s_boot_cloud_retry_cnt++;
            TAL_PR_NOTICE("ai toy -> wait cloud connect, retry=%d", s_boot_cloud_retry_cnt);
            tal_workq_start_delayed(s_boot_switch_delayed_work, 2000, LOOP_ONCE);
            return;
        }
    }
#endif

    if (TRUE == s_ai_client_ready || s_boot_switch_retry_cnt >= 3) {
        if (OPRT_OK == __ai_toy_report_switch(TRUE)) {
            s_boot_switch_reported = TRUE;
            TAL_PR_NOTICE("ai toy -> delayed report dp%d=true, ai_ready=%d", AI_TOY_DP_SWITCH, s_ai_client_ready);
        } else {
            TAL_PR_ERR("ai toy -> delayed report dp%d=true failed, ai_ready=%d", AI_TOY_DP_SWITCH, s_ai_client_ready);
        }
        return;
    }

    s_boot_switch_retry_cnt++;
    TAL_PR_NOTICE("ai toy -> boot dp%d wait ai ready, retry=%d", AI_TOY_DP_SWITCH, s_boot_switch_retry_cnt);
    tal_workq_start_delayed(s_boot_switch_delayed_work, 2000, LOOP_ONCE);
}

STATIC VOID __ai_toy_boot_switch_sync_current_net_status(VOID)
{
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
    GW_WIFI_NW_STAT_E nw_stat = 0;

    if (OPRT_OK != get_wf_gw_nw_status(&nw_stat)) {
        TAL_PR_ERR("ai toy -> get current wifi network status failed");
        return;
    }

    TAL_PR_NOTICE("ai toy -> sync current wifi network status: %d", nw_stat);
    __on_ai_toy_wf_nw_stat_cb(nw_stat);

    if (STAT_CLOUD_CONN != nw_stat && FALSE == s_boot_switch_reported && NULL != s_boot_switch_delayed_work) {
        s_boot_cloud_retry_cnt = 0;
        tal_workq_start_delayed(s_boot_switch_delayed_work, 2000, LOOP_ONCE);
    }
#endif
}

/** Report volume DP (dpid 3) to cloud. */
STATIC OPERATE_RET __ai_toy_report_volum(VOID)
{
    CHAR_T *devid = tuya_iot_get_gw_id();
    /* Build volume DP: dpid 3, type PROP_VALUE, value 0~100. */
    TY_OBJ_DP_S dp = {
        .dpid = 3,
        .type = PROP_VALUE,
        .value.dp_value = s_ai_toy->volume,
    };
    return tuya_report_dp_async(devid, &dp, 1, NULL);
}

/** Clamp external volume requests into the player-supported 0~100 range. */
STATIC UINT8_T __ai_toy_volume_clamp(INT_T value)
{
    if (value < 0) {
        return 0;
    }

    if (value > 100) {
        return 100;
    }

    return (UINT8_T)value;
}

/** Save volume and trigger_mode to KV (AI_TOY_PARA). */
STATIC OPERATE_RET __ai_toy_config_save(VOID)
{
    TAL_PR_NOTICE("ai toy -> save config");

    OPERATE_RET rt = OPRT_OK;
    CHAR_T buf[64] = {0};
    /* Serialise volume and trigger_mode as JSON and write to KV key AI_TOY_PARA. */
    snprintf(buf, sizeof(buf), "{\"volume\": %d, \"trigger_mode\":%d}", s_ai_toy->volume, s_ai_toy->cfg.trigger_mode);
    TUYA_CALL_ERR_RETURN(wd_common_write(AI_TOY_PARA, (CONST BYTE_T *)buf, strlen(buf)));

    TAL_PR_DEBUG("save ai_toy config: %s", buf);
    return rt;
}

/** Load volume and trigger_mode from KV; use defaults if read fails. */
STATIC OPERATE_RET __ai_toy_config_load(VOID)
{
    TAL_PR_NOTICE("ai toy -> load config");
    BYTE_T *value = NULL;
    UINT_T len = 0;

    /* Default volume before reading KV. */
    s_ai_toy->volume = TY_SPK_DEFAULT_VOL;

    /* Missing history is not fatal: keep defaults on first boot or after cleanup. */
    if (wd_common_read(AI_TOY_PARA, &value, &len) != OPRT_OK) {
        TAL_PR_NOTICE("ai toy -> no saved config, use defaults");
        return OPRT_OK;
    }
    TAL_PR_DEBUG("read ai_toy config: %s", value);
    ty_cJSON *root = ty_cJSON_Parse((CONST CHAR_T *)value);
    wd_common_free_data(value);
    TUYA_CHECK_NULL_RETURN(root, OPRT_FILE_READ_FAILED);

    /* Parse "volume" (0~100). */
    ty_cJSON *volum = ty_cJSON_GetObjectItem(root, "volume");
    if (volum) {
        if (volum->valueint <= 100 && volum->valueint >= 0) {
            s_ai_toy->volume = volum->valueint;
        }
    }

    /* Parse "trigger_mode" (AI_TRIGGER_MODE_HOLD ~ AI_TRIGGER_MODE_P2P). */
    ty_cJSON *trigger_mode = ty_cJSON_GetObjectItem(root, "trigger_mode");
    if (trigger_mode) {
        if (trigger_mode->valueint <= AI_TRIGGER_MODE_P2P && trigger_mode->valueint >= AI_TRIGGER_MODE_HOLD) {
            s_ai_toy->cfg.trigger_mode = trigger_mode->valueint;
        }
    }

    ty_cJSON_Delete(root);
    return OPRT_OK;
}

/** Apply volume to runtime, UI and cloud, with optional persistence for user-driven changes. */
STATIC OPERATE_RET __ai_toy_volume_apply(INT_T value, BOOL_T persist, BOOL_T report)
{
    OPERATE_RET rt = OPRT_OK;
    UINT8_T volume = __ai_toy_volume_clamp(value);

    TUYA_CHECK_NULL_RETURN(s_ai_toy, OPRT_RESOURCE_NOT_READY);

    /* Skip duplicate writes, but still allow callers to refresh persistence/report when required. */
    if (s_ai_toy->volume != volume) {
        s_ai_toy->volume = volume;
        TUYA_CALL_ERR_LOG(wukong_audio_player_set_vol(s_ai_toy->volume));
#if defined(ENABLE_TUYA_UI)
        tuya_ai_display_msg(&s_ai_toy->volume, 1, TY_DISPLAY_TP_VOLUME);
#endif
    }

    if (persist) {
        TUYA_CALL_ERR_RETURN(__ai_toy_config_save());
    }

    if (report) {
        TUYA_CALL_ERR_LOG(__ai_toy_report_volum());
    }

    return rt;
}

/* ---------------------------------------------------------------------------
 * Key and network callbacks
 * --------------------------------------------------------------------------- */

/** Net key: short press volume+, long press 5 seconds reprovision, seq press volume-. */
STATIC VOID __on_ai_toy_net_pin(UINT_T port, PUSH_KEY_TYPE_E type, INT_T cnt)
{
    TAL_PR_DEBUG("ai toy -> net pin pressed %d", type);
    switch (type) {
    case NORMAL_KEY:
        TAL_PR_DEBUG("net pin normal press trigger volume up!");
        if (s_ai_toy && s_ai_toy->volume < 100) {
            INT_T next_volume = s_ai_toy->volume;
            OPERATE_RET op_ret = OPRT_OK;

            if (s_ai_toy->volume % TOY_VOLUME_SETUP) {
                next_volume = (s_ai_toy->volume / TOY_VOLUME_SETUP + 1) * TOY_VOLUME_SETUP;
            } else {
                next_volume = s_ai_toy->volume + TOY_VOLUME_SETUP;
            }
            TAL_PR_DEBUG("volume %d", next_volume);
            /* Use the canonical volume setter so key changes share the same save/report flow. */
            op_ret = tuya_ai_toy_volume_set((UINT8_T)__ai_toy_volume_clamp(next_volume));
            if (OPRT_OK != op_ret) {
                TAL_PR_ERR("net pin volume up failed: %d", op_ret);
            }
        }
        break;

    case LONG_KEY:
        /* Long press 5s: enter smart config / reset netconfig (WiFi only). */
        TAL_PR_NOTICE("net pin long press 5s trigger netconfig reset");
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
        tuya_iot_wf_gw_fast_unactive(GWCM_OLD, WF_START_SMART_AP_CONCURRENT);
#endif
        break;

    case SEQ_KEY:
        TAL_PR_DEBUG("net pin seq press trigger volume down!");
        if (s_ai_toy && s_ai_toy->volume > 0) {
            INT_T next_volume = s_ai_toy->volume;
            OPERATE_RET op_ret = OPRT_OK;

            if (s_ai_toy->volume % TOY_VOLUME_SETUP) {
                next_volume = (s_ai_toy->volume / TOY_VOLUME_SETUP) * TOY_VOLUME_SETUP;
            } else {
                next_volume = s_ai_toy->volume - TOY_VOLUME_SETUP;
            }
            TAL_PR_DEBUG("volume %d", next_volume);
            /* Use the canonical volume setter so key changes share the same save/report flow. */
            op_ret = tuya_ai_toy_volume_set((UINT8_T)__ai_toy_volume_clamp(next_volume));
            if (OPRT_OK != op_ret) {
                TAL_PR_ERR("net pin volume down failed: %d", op_ret);
            }
        }
        break;

    default:
        break;
    }
}

/** Forward mic data to wukong audio input. */
STATIC INT_T __on_ai_toy_mic_data(UINT8_T *data, UINT16_T datalen)
{
    return wukong_ai_handle_audio_input(data, datalen);
}

#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
/** WiFi network status: update language from region/country code, LED and display, report DP on first cloud connect. */
STATIC VOID __on_ai_toy_wf_nw_stat_cb(GW_WIFI_NW_STAT_E nw_stat)
{
    /* Derive language: 0 = Chinese, 1 = English from GW region or WiFi country code ("CN" -> Chinese). */
    CONST CHAR_T *region = get_gw_region();
    if (0 == strlen(region)) {
        CHAR_T ccode[COUNTRY_CODE_LEN] = {0};
        tal_wifi_get_country_code(ccode);
        s_lang = (NULL != strstr(ccode, "CN") ? 0 : TY_AI_DEFAULT_LANG);
#if defined(ENABLE_TUYA_UI)
        tuya_ai_display_msg(&s_lang, 1, TY_DISPLAY_TP_LANGUAGE);
#endif
        TAL_PR_DEBUG("network status = %d, ccode %s, language %d", nw_stat, ccode, s_lang);
    } else {
        s_lang = (NULL != strstr(region, "AY") ? 0 : TY_AI_DEFAULT_LANG);
#if defined(ENABLE_TUYA_UI)
        tuya_ai_display_msg(&s_lang, 1, TY_DISPLAY_TP_LANGUAGE);
#endif
        TAL_PR_DEBUG("network status = %d, region %s, language %d", nw_stat, region, s_lang);
    }

    UINT8_T net_stat = 0;
    STATIC BOOL_T report_flag = FALSE;
    switch (nw_stat) {
    case STAT_UNPROVISION_AP_STA_UNCFG:
        /* Unprovisioned: show netconfig UI, LED flash 200 ms. */
#if defined(ENABLE_TUYA_UI)
        tuya_ai_display_msg(NULL, 0, TY_DISPLAY_TP_STAT_NETCFG);
        tuya_ai_display_msg(&net_stat, 1, TY_DISPLAY_TP_STAT_NET);
#endif
        tuya_ai_toy_led_flash(200);
        break;

    case STAT_STA_DISC:
        /* WiFi disconnected: update display, LED off. */
#if defined(ENABLE_TUYA_UI)
        tuya_ai_display_msg(&net_stat, 1, TY_DISPLAY_TP_STAT_NET);
#endif
        tuya_ai_toy_led_off();
        break;

    case STAT_CLOUD_CONN:
        /* First time cloud connected: report volume once, mark cloud-ready, then set LED on. */
        if (!report_flag) {
            __ai_toy_report_volum();
#ifdef ENABLE_TUYA_UI
            tuya_ai_display_msg(NULL, 0, TY_DISPLAY_TP_STAT_ONLINE);
#endif
            s_boot_cloud_connected = TRUE;
            s_boot_cloud_retry_cnt = 0;
            s_boot_switch_retry_cnt = 0;
            if (s_network_cfg_timer) {
                tal_sw_timer_stop(s_network_cfg_timer);
            }
            __ai_toy_report_boot_switch_if_ready();
            if (FALSE == s_boot_switch_reported && s_boot_switch_delayed_work) {
                tal_workq_start_delayed(s_boot_switch_delayed_work, 2000, LOOP_ONCE);
            }
            report_flag = TRUE;
        }
        net_stat = 1;
        #ifdef ENABLE_TUYA_UI  
        tuya_ai_display_msg(&net_stat, 1, TY_DISPLAY_TP_STAT_NET);
        #endif
        tuya_ai_toy_led_on();
        break;
    }

    s_ai_toy->nw_stat = nw_stat;
}
#endif

/* ---------------------------------------------------------------------------
 * Wukong AI event and task
 * --------------------------------------------------------------------------- */

STATIC VOID __ai_toy_mcp_init_work(VOID_T *data)
{
    (VOID)data;

    s_mcp_init_scheduled = FALSE;
    if (TRUE == s_mcp_inited) {
        return;
    }

    if (OPRT_OK == tuya_ai_toy_mcp_init()) {
        s_mcp_inited = TRUE;
        TAL_PR_NOTICE("ai toy -> mcp init success (async)");
    } else {
        TAL_PR_ERR("ai toy -> mcp init failed (async)");
    }
}

/** Downlink event from wukong: forward to wukong_ai_handle_event. */
STATIC VOID __on_ai_toy_wukong_ai_event(WUKONG_AI_EVENT_T *event)
{
    wukong_ai_handle_event(event, 0);
}

/** Main AI toy task loop: handle wukong task and sleep. */
STATIC VOID __ai_toy_task(VOID *args)
{
    while (tal_thread_get_state(s_ai_toy->toy_task) == THREAD_STATE_RUNNING) {
        wukong_ai_handle_task(NULL, 0);
        tal_system_sleep(20);
    }
}

/** Audio trigger key: seq = stop player, switch trigger mode and alert; else forward to wukong key handler. */
STATIC VOID __on_ai_toy_audio_trigger_pin(UINT_T port, PUSH_KEY_TYPE_E type, INT_T cnt)
{
    if (SEQ_KEY == type) {
        /* Double press: stop all playback, send chat break, cycle trigger mode, save, play mode alert. */
#if defined(ENABLE_TUYA_UI) && (ENABLE_TUYA_UI == 1)
        tuya_ai_display_msg(NULL, 0, TY_DISPLAY_TP_STAT_WAKEUP);
#endif
        wukong_audio_player_stop(AI_PLAYER_ALL);
        tuya_ai_agent_event(AI_EVENT_CHAT_BREAK, 0);
        s_ai_toy->cfg.trigger_mode = wukong_ai_mode_switch(s_ai_toy->cfg.trigger_mode);
        __ai_toy_config_save();
        wukong_audio_player_alert(AI_TOY_ALERT_TYPE_LONG_KEY_TALK + s_ai_toy->cfg.trigger_mode, TRUE);
        return;
    }
    if (NORMAL_KEY == type || LONG_KEY == type) {
#if defined(ENABLE_TUYA_UI) && (ENABLE_TUYA_UI == 1)
        tuya_ai_display_msg(NULL, 0, TY_DISPLAY_TP_STAT_WAKEUP);
#endif
        tuya_ai_toy_wakeup_dp_report(TRUE);
    }
    /* Single/long press: pass to wukong key handler (e.g. hold to talk). */
    wukong_ai_handle_key(&type, 0);
}

/** KWS wakeup event: set wakeup index and forward to wukong. */
STATIC OPERATE_RET __on_ai_toy_audio_kws(VOID_T *data)
{
    if (data) {
        INT_T idx = (INT_T)data;
        __s_wakeup_flag = idx;
    }

    if (AI_TRIGGER_MODE_FREE == tuya_ai_toy_trigger_mode_get() && ai_free_wakeup_is_active()) {
        TAL_PR_DEBUG("ignore kws wakeup while free chat is active");
        return OPRT_OK;
    }

#if defined(ENABLE_TUYA_UI) && (ENABLE_TUYA_UI == 1)
    tuya_ai_display_msg(NULL, 0, TY_DISPLAY_TP_STAT_WAKEUP);
#endif
    tuya_ai_toy_wakeup_dp_report(TRUE);
    return wukong_ai_handle_wakeup(NULL, 0);
}

/** VAD state change: forward to wukong. */
STATIC OPERATE_RET __on_ai_toy_vad_change(VOID *data)
{
    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);
    wukong_ai_handle_vad(data, 0);
    return OPRT_OK;
}

/** AI client connected: forward to wukong. */
STATIC OPERATE_RET __on_ai_toy_ai_client_run(VOID_T *data)
{
    TAL_PR_NOTICE("ai toy -> connected to server");
    s_ai_client_ready = TRUE;
    __ai_toy_report_boot_switch_if_ready();
    wukong_ai_handle_client(NULL, 0);
    return OPRT_OK;
}

/* ---------------------------------------------------------------------------
 * Wukong agent init/deinit and OTA callbacks
 * --------------------------------------------------------------------------- */

STATIC OPERATE_RET __ai_toy_wukong_ai_agent_init(VOID)
{
    OPERATE_RET rt = OPRT_OK;

    /* Register downlink event callback and init player. */
    TUYA_CALL_ERR_LOG(wukong_ai_agent_init(__on_ai_toy_wukong_ai_event));
    TUYA_CALL_ERR_LOG(wukong_audio_player_init());

    /* Audio input: board (16k, mono, 80 ms slice, VAD by trigger mode) or UART. */
    WUKONG_AUDIO_INPUT_CFG_T audio_cfg = {0};
#if defined(USING_BOARD_AUDIO_INPUT) && (USING_BOARD_AUDIO_INPUT == 1)
    audio_cfg.type = WUKONG_AUDIO_USING_BOARD;
    audio_cfg.board.sample_rate    = TKL_AUDIO_SAMPLE_16K;
    audio_cfg.board.sample_bits    = TKL_AUDIO_DATABITS_16;
    audio_cfg.board.channel        = TKL_AUDIO_CHANNEL_MONO;
    audio_cfg.board.slice_ms       = AI_AUDIO_SLICE_TIME;
    /* HOLD trigger -> manual VAD; otherwise auto VAD (e.g. one-shot, free talk). */
    audio_cfg.board.vad_mode       = (s_ai_toy->cfg.trigger_mode == AI_TRIGGER_MODE_HOLD) ? WUKONG_AUDIO_VAD_MANIAL : WUKONG_AUDIO_VAD_AUTO;
    audio_cfg.board.vad_off_ms     = 1000;
    audio_cfg.board.vad_active_ms  = 200;
    audio_cfg.board.spk_io         = s_ai_toy->cfg.spk_en_pin;
    audio_cfg.board.spk_io_level   = TUYA_GPIO_LEVEL_LOW;
    audio_cfg.board.output_cb      = __on_ai_toy_mic_data;
#elif defined(USING_UART_AUDIO_INPUT) && (USING_UART_AUDIO_INPUT == 1)
    audio_cfg.type = WUKONG_AUDIO_USING_UART;
    audio_cfg.uart.mic_upload = __on_ai_toy_mic_data;
#endif
    TUYA_CALL_ERR_LOG(wukong_audio_input_init(&audio_cfg));

    wukong_audio_player_set_vol(s_ai_toy->volume);
    TUYA_CALL_ERR_LOG(wukong_kws_default_init());
    return rt;
}

STATIC OPERATE_RET __ai_toy_wukong_ai_agent_deinit(VOID)
{
    OPERATE_RET rt = OPRT_OK;

    TUYA_CALL_ERR_LOG(wukong_audio_input_deinit());
    TUYA_CALL_ERR_LOG(wukong_audio_player_deinit());
    TUYA_CALL_ERR_LOG(wukong_ai_agent_deinit());

    return rt;
}

/** Unsubscribe events, delete task, deinit MCP/monitor and wukong agent. */
STATIC OPERATE_RET __ai_toy_stop(VOID)
{
    TAL_PR_NOTICE("ai toy -> stop");
    OPERATE_RET rt = OPRT_OK;

    /* Unsubscribe all events registered in __ai_toy_start. */
    ty_unsubscribe_event(EVENT_AI_CLIENT_RUN,      "ai_toy", __on_ai_toy_ai_client_run);
    ty_unsubscribe_event(EVENT_WUKONG_KWS_WAKEUP,  "ai_toy", __on_ai_toy_audio_kws);
    ty_unsubscribe_event(EVENT_AUDIO_VAD,          "ai_toy", __on_ai_toy_vad_change);

    if (s_ai_toy->toy_task) {
        TUYA_CALL_ERR_LOG(tal_thread_delete(s_ai_toy->toy_task));
    }

    if (s_mcp_init_scheduled) {
        TUYA_CALL_ERR_LOG(tal_workq_cancel(WORKQ_SYSTEM, __ai_toy_mcp_init_work, NULL));
        s_mcp_init_scheduled = FALSE;
    }

    if (s_boot_switch_delayed_work) {
        TUYA_CALL_ERR_LOG(tal_workq_stop_delayed(s_boot_switch_delayed_work));
        TUYA_CALL_ERR_LOG(tal_workq_cancel_delayed(s_boot_switch_delayed_work));
    }

    if (s_mcp_inited) {
        TUYA_CALL_ERR_LOG(tuya_ai_toy_mcp_deinit());
        s_mcp_inited = FALSE;
    }

#if defined(ENABLE_APP_AI_MONITOR) && (ENABLE_APP_AI_MONITOR == 1)
    TUYA_CALL_ERR_LOG(tuya_ai_monitor_deinit());
#endif

    TUYA_CALL_ERR_LOG(__ai_toy_wukong_ai_agent_deinit());
    TAL_PR_NOTICE("ai toy -> stop success");
    return rt;
}

/** Subscribe events, init wukong agent, register WF NW cb, start monitor/MCP/mode and task. */
STATIC OPERATE_RET __ai_toy_start(VOID)
{
    OPERATE_RET rt = OPRT_OK;
    /* Subscribe AI client, KWS wakeup, VAD. */
    ty_subscribe_event(EVENT_AI_CLIENT_RUN,      "ai_toy", __on_ai_toy_ai_client_run, SUBSCRIBE_TYPE_NORMAL);
    ty_subscribe_event(EVENT_WUKONG_KWS_WAKEUP,  "ai_toy", __on_ai_toy_audio_kws, SUBSCRIBE_TYPE_NORMAL);
    ty_subscribe_event(EVENT_AUDIO_VAD,          "ai_toy", __on_ai_toy_vad_change, SUBSCRIBE_TYPE_NORMAL);

    TUYA_CALL_ERR_GOTO(__ai_toy_wukong_ai_agent_init(), __error);
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
    tuya_iot_reg_get_wf_nw_stat_cb(__on_ai_toy_wf_nw_stat_cb);
#endif

#if defined(ENABLE_APP_AI_MONITOR) && (ENABLE_APP_AI_MONITOR == 1)
    ai_monitor_config_t monitor_cfg = AI_MONITOR_CFG_DEFAULT;
    TUYA_CALL_ERR_GOTO(tuya_ai_monitor_init(&monitor_cfg), __error);
#endif

    if (FALSE == s_mcp_inited && FALSE == s_mcp_init_scheduled) {
        TUYA_CALL_ERR_GOTO(tal_workq_schedule(WORKQ_SYSTEM, __ai_toy_mcp_init_work, NULL), __error);
        s_mcp_init_scheduled = TRUE;
    }

    if (NULL == s_boot_switch_delayed_work) {
        TUYA_CALL_ERR_GOTO(tal_workq_init_delayed(WORKQ_SYSTEM, __ai_toy_boot_switch_delayed_work_cb, NULL,
                                                  &s_boot_switch_delayed_work), __error);
    }

#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
    __ai_toy_boot_switch_sync_current_net_status();
#endif

    TUYA_CALL_ERR_GOTO(wukong_ai_mode_init(), __error);

    /* Create and start the AI toy state task (runs __ai_toy_task). */
    THREAD_CFG_T thrd_cfg = {
        .priority = THREAD_PRIO_5,
        .stackDepth = 3* 1024,
        .thrdname = "ai_toy_state",
        #ifdef ENABLE_EXT_RAM
        .psram_mode = 1,
        #endif            
    };
    TUYA_CALL_ERR_GOTO(tal_thread_create_and_start(&s_ai_toy->toy_task, NULL, NULL, __ai_toy_task, NULL, &thrd_cfg), __error);


    TAL_PR_NOTICE("ai toy -> start success");
    return OPRT_OK;

__error:
    TAL_PR_ERR("ai toy -> start failed, stop ai agent");
    __ai_toy_stop();
    return rt;
}

/* ---------------------------------------------------------------------------
 * Idle and low-power timers
 * --------------------------------------------------------------------------- */

/** Idle timer: if not playing, notify idle; else restart timer (TOY_IDLE_TIMEOUT ms). */
STATIC VOID __on_ai_toy_idle_timer(TIMER_ID timer_id, VOID_T *arg)
{
    TAL_PR_NOTICE("ai toy -> idle timer out, will check and enter idle status");
    if (wukong_audio_player_is_playing()) {
        TAL_PR_NOTICE("ai toy -> player is playing, idle timer reset");
        tal_sw_timer_start(s_ai_toy->idle_timer, TOY_IDLE_TIMEOUT, TAL_TIMER_ONCE);
        return;
    }
    /* No playback: notify wukong to enter idle state. */
    wukong_ai_notify_idle(NULL, 0);
    __ai_toy_report_idle(TRUE);
}

STATIC VOID __on_ai_toy_network_cfg_timer(TIMER_ID timer_id, VOID_T *arg)
{
    (VOID)timer_id;
    (VOID)arg;

    if (TRUE == s_boot_cloud_connected || TRUE == s_network_cfg_prompted) {
        return;
    }

    s_network_cfg_prompted = TRUE;
    TAL_PR_NOTICE("ai toy -> boot network config prompt");
    wukong_audio_player_alert(AI_TOY_ALERT_TYPE_NETWORK_CFG, FALSE);
}

/** Configure GPIO as wakeup source for deep sleep.
 * The audio trigger key is initialized as active-low with an internal pull-up,
 * so the deep-sleep wake source must also use pull-up + falling-edge wake.
 */
STATIC VOID __ai_toy_set_wakeup_source(UINT32_T pin)
{
    TAL_PR_NOTICE("ai toy -> set wakeup pin %d", pin);
    /* Keep the wake key in the same idle state as the runtime key config. */
    TUYA_GPIO_BASE_CFG_T io_cfg;
    io_cfg.direct = TUYA_GPIO_INPUT;
    io_cfg.mode = TUYA_GPIO_PULLUP;
    io_cfg.level = TUYA_GPIO_LEVEL_HIGH;
    tkl_gpio_init(pin, &io_cfg);

    /* Active-low key: wake when the line falls on key press. */
    TUYA_WAKEUP_SOURCE_BASE_CFG_T cfg;
    cfg.source = TUYA_WAKEUP_SOURCE_GPIO;
    cfg.wakeup_para.gpio_param.gpio_num = pin;
    cfg.wakeup_para.gpio_param.level = TUYA_GPIO_WAKEUP_FALL;
    tkl_wakeup_source_set(&cfg);
    tal_system_sleep(200);
}

/** Low-power timer: if not playing and TY_AI_DEFAULT_LOWP_MODE defined, enter deep sleep or light sleep. */
STATIC VOID __on_ai_toy_lowpower_timer(TIMER_ID timer_id, VOID_T *arg)
{
#if defined(TY_AI_DEFAULT_LOWP_MODE)
    TAL_PR_NOTICE("ai toy -> lowpower timer out, will check and enter idle status");
    if (wukong_audio_player_is_playing()) {
        TAL_PR_NOTICE("ai toy -> player is playing, lowpower timer reset");
        tal_sw_timer_start(s_ai_toy->lowpower_timer, TOY_LOWPOWER_TIMEOUT, TAL_TIMER_ONCE);
        return;
    }

    OPERATE_RET rt = OPRT_OK;
    if (TY_AI_DEFAULT_LOWP_MODE == TUYA_CPU_DEEP_SLEEP) {
        /* Deep sleep: set audio trigger pin as wakeup, then enter deep sleep. */
        __ai_toy_set_wakeup_source(s_ai_toy->cfg.audio_trigger_pin);
        tal_cpu_sleep_mode_set(1, TUYA_CPU_DEEP_SLEEP);
    } else if (TY_AI_DEFAULT_LOWP_MODE == TUYA_CPU_SLEEP) {
        /* Light sleep: uninit battery, turn off speaker and LED, stop AI agent, then enable CPU/WiFi LP. */
#if defined(TUYA_AI_TOY_BATTERY_ENABLE) && (TUYA_AI_TOY_BATTERY_ENABLE == 1)
        tuya_ai_toy_battery_uninit();
#endif
        tkl_gpio_write(s_ai_toy->cfg.spk_en_pin, TUYA_GPIO_LEVEL_LOW);
        tkl_gpio_write(s_ai_toy->cfg.led_pin, TUYA_GPIO_LEVEL_LOW);

        __ai_toy_stop();

        rt = tal_cpu_lp_enable();
        rt |= tal_wifi_lp_enable();
        s_ai_toy->lp_stat = TRUE;
        TAL_PR_DEBUG("tal_cpu_lp_enable rt=%d", rt);
    }
#endif
}

/* ---------------------------------------------------------------------------
 * Public API (see tuya_ai_toy.h)
 * --------------------------------------------------------------------------- */

OPERATE_RET tuya_ai_toy_destroy(VOID)
{
    TAL_PR_NOTICE("ai toy -> destroy");
    TUYA_CHECK_NULL_RETURN(s_ai_toy, OPRT_OK);

    if (s_ai_toy->idle_timer) {
        tal_sw_timer_delete(s_ai_toy->idle_timer);
    }
    if (s_ai_toy->lowpower_timer) {
        tal_sw_timer_delete(s_ai_toy->lowpower_timer);
    }
    if (s_network_cfg_timer) {
        tal_sw_timer_delete(s_network_cfg_timer);
        s_network_cfg_timer = NULL;
    }
    tal_free(s_ai_toy);
    s_ai_toy = NULL;
    TAL_PR_NOTICE("ai toy -> destroy success");
    return OPRT_OK;
}

OPERATE_RET tuya_ai_toy_init(TY_AI_TOY_CFG_T *cfg)
{
    TUYA_CHECK_NULL_RETURN(cfg, OPRT_INVALID_PARM);

    OPERATE_RET rt = OPRT_OK;
    TAL_PR_NOTICE("ai toy -> init");

    /* Alloc context: PSRAM if enabled, else heap. */
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    s_ai_toy = tal_psram_malloc(SIZEOF(TY_AI_TOY_T));
#else
    s_ai_toy = tal_malloc(SIZEOF(TY_AI_TOY_T));
#endif
    TUYA_CHECK_NULL_RETURN(s_ai_toy, OPRT_MALLOC_FAILED);
    memset(s_ai_toy, 0, SIZEOF(TY_AI_TOY_T));
    memcpy(&s_ai_toy->cfg, cfg, sizeof(TY_AI_TOY_CFG_T));
    s_ai_toy->wakeup_stat = FALSE;
    s_ai_client_ready = FALSE;
    s_boot_cloud_connected = FALSE;
    s_boot_switch_reported = FALSE;
    s_boot_switch_retry_cnt = 0;
    s_boot_cloud_retry_cnt = 0;
    s_network_cfg_prompted = FALSE;
    TUYA_CALL_ERR_LOG(__ai_toy_config_load());

    /* LED and two keys: audio trigger key, and dedicated netconfig key (long press 5s). */
    TUYA_CALL_ERR_GOTO(tuya_ai_toy_led_init(s_ai_toy->cfg.led_pin), __error);
    TUYA_CALL_ERR_GOTO(tuya_ai_toy_key_init(s_ai_toy->cfg.audio_trigger_pin, TRUE, SEQ_KEY_TIME, LONG_KEY_TIME, __on_ai_toy_audio_trigger_pin), __error);
    TUYA_CALL_ERR_GOTO(tuya_ai_toy_key_init(s_ai_toy->cfg.net_pin, TRUE, SEQ_KEY_TIME, NETCFG_LONG_KEY_TIME, __on_ai_toy_net_pin), __error);

#if defined(ENABLE_TUYA_CAMERA) && (ENABLE_TUYA_CAMERA == 1)
    TAL_PR_NOTICE("ai toy -> init camera");
    tuya_ai_toy_camera_init(AI_TOY_CAMERA_LOCAL);
#if defined(ENABLE_AI_MODE_P2P) && (ENABLE_AI_MODE_P2P == 1)
    tuya_p2p_app_start();
#endif
#endif

#if defined(TUYA_AI_TOY_BATTERY_ENABLE) && (TUYA_AI_TOY_BATTERY_ENABLE == 1)
    TAL_PR_NOTICE("ai toy -> init battery");
    TUYA_CALL_ERR_LOG(tuya_ai_toy_battery_init());
#endif

#if defined(ENABLE_CELLULAR_DONGLE) && (ENABLE_CELLULAR_DONGLE == 1)
    TAL_PR_NOTICE("ai toy -> init cellular");
    TUYA_CALL_ERR_LOG(tuya_ai_cellular_init());
#endif

    /* Idle timer: no activity for TOY_IDLE_TIMEOUT -> idle. Low-power timer: idle for TOY_LOWPOWER_TIMEOUT -> sleep. */
    TUYA_CALL_ERR_GOTO(tal_sw_timer_create(__on_ai_toy_idle_timer, s_ai_toy, &s_ai_toy->idle_timer), __error);
    TUYA_CALL_ERR_GOTO(tal_sw_timer_create(__on_ai_toy_lowpower_timer, s_ai_toy, &s_ai_toy->lowpower_timer), __error);
    TUYA_CALL_ERR_GOTO(tal_sw_timer_create(__on_ai_toy_network_cfg_timer, s_ai_toy, &s_network_cfg_timer), __error);

    TUYA_CALL_ERR_GOTO(__ai_toy_start(), __error);
    if (s_network_cfg_timer) {
        tal_sw_timer_start(s_network_cfg_timer, TOY_NETCFG_PROMPT_TIMEOUT, TAL_TIMER_ONCE);
    }
    wukong_audio_player_alert(AI_TOY_ALERT_TYPE_NOT_ACTIVE, FALSE);
    TAL_PR_NOTICE("ai toy -> init success");
    return rt;

__error:
    tuya_ai_toy_destroy();
    return rt;
}

VOID tuya_ai_toy_dp_process(CONST TY_RECV_OBJ_DP_S *dp)
{
    for (UINT_T index = 0; index < dp->dps_cnt; index++) {
        /* dpid 8: robot action (T5AI_BOARD_ROBOT only). */
        if (dp->dps[index].dpid == 8) {
#if defined(T5AI_BOARD_ROBOT) && (T5AI_BOARD_ROBOT == 1)
            tuya_robot_action_set(dp->dps[index].value.dp_enum);
#endif
            TAL_PR_DEBUG("SOC Rev DP Obj Cmd dpid:%d type:%d value:%d", dp->dps[index].dpid, dp->dps[index].type, dp->dps[index].value.dp_enum);
        } else if (dp->dps[index].dpid == 3 && dp->dps[index].type == PROP_VALUE) {
            /* dpid 3: volume (0~100). Update local volume, player, display and report. */
            TAL_PR_DEBUG("SOC Rev DP Obj Cmd dpid:%d type:%d value:%d", dp->dps[index].dpid, dp->dps[index].type, dp->dps[index].value.dp_value);
            if (dp->dps[index].value.dp_value <= 100 && dp->dps[index].value.dp_value >= 0) {
                OPERATE_RET op_ret = OPRT_OK;

                /* Persist App-side volume changes through the same canonical setter as local keys. */
                op_ret = tuya_ai_toy_volume_set((UINT8_T)dp->dps[index].value.dp_value);
                if (OPRT_OK != op_ret) {
                    TAL_PR_ERR("dp volume set failed: %d", op_ret);
                }
            }
        }
    }
}

UINT8_T tuya_ai_toy_get_lang()
{
    return s_lang;
}

AI_TRIGGER_MODE_E tuya_ai_toy_trigger_mode_get(VOID)
{
    return s_ai_toy->cfg.trigger_mode;
}

VOID tuya_ai_toy_trigger_mode_set(AI_TRIGGER_MODE_E mode)
{
    s_ai_toy->cfg.trigger_mode = mode;
}

VOID tuya_ai_toy_lowpower_timer_ctrl(BOOL_T enable)
{
    TAL_PR_DEBUG("[====ai_toy] lowpower timer ctrl enable:%d", enable);
    if (!s_ai_toy || !s_ai_toy->lowpower_timer) {
        return;
    }

    if (enable) {
        tal_sw_timer_start(s_ai_toy->lowpower_timer, TOY_LOWPOWER_TIMEOUT, TAL_TIMER_ONCE);
    } else {
        tal_sw_timer_stop(s_ai_toy->lowpower_timer);
    }
}

VOID tuya_ai_toy_idle_timer_ctrl(BOOL_T enable)
{
    TAL_PR_DEBUG("[====ai_toy] idle timer ctrl enable:%d", enable);
    if (enable) {
        __ai_toy_report_idle(FALSE);
        tal_sw_timer_start(s_ai_toy->idle_timer, TOY_IDLE_TIMEOUT, TAL_TIMER_ONCE);
    } else {
        tal_sw_timer_stop(s_ai_toy->idle_timer);
    }
}

OPERATE_RET tuya_ai_toy_volume_set(UINT8_T value)
{
    /* Route all external setters through the same persistence/report path. */
    return __ai_toy_volume_apply(value, TRUE, TRUE);
}

UINT8_T tuya_ai_toy_volume_get(VOID)
{
    /* Fall back to the build default before runtime init finishes. */
    if (NULL == s_ai_toy) {
        return TY_SPK_DEFAULT_VOL;
    }

    return s_ai_toy->volume;
}
