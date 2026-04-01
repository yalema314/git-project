/*
 * tuya_ai_battery.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "tuya_ai_battery.h"
#include "tuya_ai_toy.h"
#include "tuya_iot_com_api.h"
#include "tal_log.h"
#include "tal_system.h"
#include "tal_memory.h"
#include "tal_thread.h"
#include "tal_sw_timer.h"
#include "tal_thread.h"
#include "tkl_pinmux.h"
#include "tkl_adc.h"
#include "uni_log.h"
#include "base_event.h"
#include "smart_frame.h"
#if defined(ENABLE_TUYA_UI) && (ENABLE_TUYA_UI == 1)
#include "tuya_ai_display.h"
#endif
#if defined(ENABLE_CELLULAR_DONGLE) && (ENABLE_CELLULAR_DONGLE == 1)
#include "tal_cellular.h"
#endif

#if defined(TUYA_AI_TOY_BATTERY_ENABLE) || TUYA_AI_TOY_BATTERY_ENABLE == 1

#define ADC_CONV_TIMES                     8
#define BATTERY_REPORT_PERIOD_MS           (1 * 1000)
#define BATTERY_ADC_IDLE_DELAY_MS          (15 * 1000)
#define BATTERY_ADC_CHARGING_DELAY_MS      (5 * 1000)
#define BATTERY_ADC_WARMUP_TIMES           15
#define BATTERY_ADC_WARMUP_DELAY_MS        20
#define CAPACITY_CONFIDENCE_INTERVAL       12
#define CAPACITY_CONFIDENCE_INTERVAL_DELAY 5
#define CAPACITY_CONFIDENCE_TRIM_CNT       2
#define CAPACITY_STABLE_DIFF               4
#define BATTERY_CAPACITY_JUMP_GUARD        15
#define BATTERY_CHARGE_DONE_CAPACITY       100

/* BAT -> 806K -> GND, so VBAT = VADC * (806K + 2M) / 2M. */
#define BATTERY_ADC_DIVIDER_UPPER_RES_KOHM 806U
#define BATTERY_ADC_DIVIDER_LOWER_RES_KOHM 2000U
#define BATTERY_ADC_DIVIDER_NUMERATOR      (BATTERY_ADC_DIVIDER_UPPER_RES_KOHM + BATTERY_ADC_DIVIDER_LOWER_RES_KOHM)
#define BATTERY_ADC_DIVIDER_DENOMINATOR    BATTERY_ADC_DIVIDER_UPPER_RES_KOHM


typedef enum {
    BATTERY_NO_CHARGE,
    BATTERY_CHARGEING,
    BATTERY_CHARGE_DONE,
} BATTERY_CHAEGE_STATUS;

STATIC ai_battery_conf_t batt_conf = {
    .mode = AI_BATTERY_MODE_MAX,
    .cb = NULL,
};
STATIC INT32_T adc_chan = 4;
STATIC UINT8_T last_capacity = 0xff;
STATIC TIMER_ID sw_timer_id = NULL;
STATIC THREAD_HANDLE __vol_conv_thread_handle = NULL;
STATIC BOOL_T is_running = FALSE;
STATIC BOOL_T is_stop = TRUE;
STATIC BOOL_T s_is_battery_low = FALSE;
// STATIC BOOL_T s_is_charging = FALSE;
STATIC UINT8_T last_report_capacity = 0;
STATIC UINT8_T last_dp2_report_capacity = 0xff;
STATIC INT_T last_report_charge_status = -1;
STATIC INT_T s_last_ui_charge_status = -1;
STATIC BOOL_T s_battery_report_baselined = FALSE;
STATIC TY_BATTERY_CB s_battery_cb = NULL;

STATIC TUYA_GPIO_LEVEL_E charge_check_level = TUYA_GPIO_LEVEL_LOW;

STATIC BATTERY_CHAEGE_STATUS __tuya_ai_toy_charge_status_get(BOOL_T is_charging, UINT8_T current_capacity)
{
    if (!is_charging) {
        return BATTERY_NO_CHARGE;
    }

    if ((current_capacity != 0xff) && (current_capacity >= BATTERY_CHARGE_DONE_CAPACITY)) {
        return BATTERY_CHARGE_DONE;
    }

    return BATTERY_CHARGEING;
}

STATIC CONST CHAR_T *__battery_charge_status_str(BATTERY_CHAEGE_STATUS status)
{
    switch (status) {
    case BATTERY_NO_CHARGE:
        return "no_charge";
    case BATTERY_CHARGEING:
        return "charging";
    case BATTERY_CHARGE_DONE:
        return "charge_done";
    default:
        return "unknown";
    }
}

// IP5306 + 4.2V 单节锂电电压与容量映射表
voltage_cap_map bvc_map[] = {
    {.v = 4166, .c = 100},  // 满电实测电压
    {.v = 4000, .c =  90},  // 高电量区
    {.v = 3900, .c =  80},  // 放电平台上沿
    {.v = 3800, .c =  70},  // 常用高电量区
    {.v = 3700, .c =  60},  // 标称电压附近
    {.v = 3600, .c =  50},  // 中电量区
    {.v = 3500, .c =  40},  // 中低电量区
    {.v = 3400, .c =  30},  // 低电量预警区
    {.v = 3300, .c =  20},  // 低电量区
    {.v = 3200, .c =  10},  // 临近关机区
    {.v = 3000, .c =   0}   // 软件按0%处理，避免贴着硬件保护下限
};


/**
 * @brief get map size
 *
 * @return size
 */
INT_T tuya_ai_vol2cap_map_size(VOID)
{
    return sizeof(bvc_map) / sizeof(bvc_map[0]);
}

STATIC UINT32_T __adc_pin_voltage_to_battery_voltage(UINT32_T adc_pin_voltage)
{
    return (adc_pin_voltage * BATTERY_ADC_DIVIDER_NUMERATOR) /
           BATTERY_ADC_DIVIDER_UPPER_RES_KOHM;
}

STATIC VOID __sort_capacity_samples(UINT8_T *samples, UINT8_T count)
{
    INT_T i;
    INT_T j;

    for (i = 1; i < count; i++) {
        UINT8_T value = samples[i];
        for (j = i - 1; j >= 0 && samples[j] > value; j--) {
            samples[j + 1] = samples[j];
        }
        samples[j + 1] = value;
    }
}

STATIC UINT8_T __capacity_abs_diff(UINT8_T lhs, UINT8_T rhs)
{
    return (lhs >= rhs) ? (lhs - rhs) : (rhs - lhs);
}

/**
 * @brief adc value to capacity
 *
 * @param[in] :
 *
 * @return none
 */
UINT8_T __voltage_conv_cap(INT_T voltage, voltage_cap_map *map, UINT8_T size)
{
    if ((map == NULL) || (size == 0)) {
        return 0;
    }

    if (voltage >= map[0].v) {
        return map[0].c;
    }

    if (voltage <= map[size - 1].v) {
        return map[size - 1].c;
    }

    for (INT_T i = 1; i < size; i++) {
        if (voltage >= map[i].v) {
            UINT32_T high_v = map[i - 1].v;
            UINT32_T low_v = map[i].v;
            UINT32_T high_c = map[i - 1].c;
            UINT32_T low_c = map[i].c;
            UINT32_T delta_v = high_v - low_v;
            UINT32_T delta_c = high_c - low_c;
            UINT32_T offset_v = voltage - low_v;

            if (delta_v == 0) {
                return low_c;
            }

            return low_c + (offset_v * delta_c + (delta_v / 2)) / delta_v;
        }
    }

    return map[size - 1].c;
}

STATIC UINT8_T __adc_conv_capacity(ai_battery_conf_t *bc)
{
    OPERATE_RET ret = OPRT_OK;
    INT_T i;
    INT_T sum = 0;
    INT_T max = 0;
    INT_T min = 0;
    UINT32_T cur_vol = 0, adc_sample = 0;
    INT32_T buf[ADC_CONV_TIMES];
    UINT8_T capacity = 0;

    sum = 0;
    memset(buf, 0, sizeof(INT32_T) * ADC_CONV_TIMES);

    ret = tkl_adc_read_voltage(0, buf, ADC_CONV_TIMES);
    if (ret != OPRT_OK) {
        PR_ERR("tkl_adc_read_voltage failed, ret=%d", ret);
        return last_capacity != 0xff ? last_capacity : 0;
        TAL_PR_NOTICE("adc=%u",ret);
    }

    // 1, discard a high/low outlier in the current ADC burst.
    for (i = 0; i < ADC_CONV_TIMES; i++) {
        sum += buf[i];

        if (i == 0 || max < buf[i]) {
            max = buf[i];
        }

        if (i == 0 || min > buf[i]) {
            min = buf[i];
        }

        PR_TRACE("adc[%d]=%d mV", i, buf[i]);
    }
    PR_TRACE("adc stats: sum=%d, max=%d, min=%d, count=%d", sum, max, min, ADC_CONV_TIMES);

    if (ADC_CONV_TIMES > 2) {
        sum -= max;
        sum -= min;
        adc_sample = sum / (ADC_CONV_TIMES - 2);
    } else {
        adc_sample = sum / ADC_CONV_TIMES;
    }
    PR_TRACE("adc avg: %u mV", adc_sample);

    // 2, restore VBAT by the actual divider in the schematic: 806K / 2M.
    cur_vol = __adc_pin_voltage_to_battery_voltage(adc_sample);
    PR_TRACE("battery vol (after divider): %u mV", cur_vol);

#if defined(ENABLE_CELLULAR_DONGLE) && (ENABLE_CELLULAR_DONGLE == 1)
    tal_cellular_get_volt(&cur_vol);
#endif

    capacity = __voltage_conv_cap(cur_vol, bc->adc.map, bc->adc.map_size);
    TAL_PR_NOTICE("adc avg=%u mV, battery vol=%u mV, capacity=%u", adc_sample, cur_vol, capacity);
    return capacity;
}

/**
 * @brief software timer callback
 *
 * @param[in] :
 *
 * @return none
 */
STATIC VOID_T __timer_cb(TIMER_ID timer_id, VOID_T *arg)
{
    ai_battery_conf_t *bc = (ai_battery_conf_t *)arg;
    if (bc->cb != NULL) {
        bc->cb(last_capacity);
    }
}

STATIC VOID __battery_monitor_timer_stop(TIMER_ID timer_id)
{
    PR_NOTICE("stop and delete battery monitor timer");
    tal_sw_timer_stop(timer_id);
    tal_sw_timer_delete(timer_id);
}

STATIC UINT8_T __adc_confidence(ai_battery_conf_t *bc, UINT8_T *out)
{
    INT_T i;
    UINT8_T capacity[CAPACITY_CONFIDENCE_INTERVAL];
    UINT32_T sum = 0;
    UINT8_T start = 0;
    UINT8_T end = CAPACITY_CONFIDENCE_INTERVAL;

    for (i = 0; i < CAPACITY_CONFIDENCE_INTERVAL; i++) {
        capacity[i] = __adc_conv_capacity(bc);
        tal_system_sleep(CAPACITY_CONFIDENCE_INTERVAL_DELAY);
    }

    __sort_capacity_samples(capacity, CAPACITY_CONFIDENCE_INTERVAL);

    if ((CAPACITY_CONFIDENCE_INTERVAL > (CAPACITY_CONFIDENCE_TRIM_CNT * 2)) && (CAPACITY_CONFIDENCE_TRIM_CNT > 0)) {
        start = CAPACITY_CONFIDENCE_TRIM_CNT;
        end = CAPACITY_CONFIDENCE_INTERVAL - CAPACITY_CONFIDENCE_TRIM_CNT;
    }

    for (i = start; i < end; i++) {
        sum += capacity[i];
    }

    *out = sum / (end - start);

    return __capacity_abs_diff(capacity[CAPACITY_CONFIDENCE_INTERVAL - 1], capacity[0]) <= CAPACITY_STABLE_DIFF;
}

STATIC BOOL_T __battery_capacity_apply(UINT8_T sample_capacity, BOOL_T is_charging)
{
    UINT8_T prev_capacity = last_capacity;

    if (sample_capacity == 0xff) {
        return FALSE;
    }

    if (last_capacity == 0xff) {
        last_capacity = sample_capacity;
        return TRUE;
    }

    if (is_charging) {
        if ((sample_capacity > last_capacity) || (sample_capacity >= BATTERY_CHARGE_DONE_CAPACITY)) {
            last_capacity = sample_capacity;
        }
    } else {
        if (sample_capacity > last_capacity) {
            PR_TRACE("ignore battery rise without charging: %u -> %u", last_capacity, sample_capacity);
            return FALSE;
        }

        if (__capacity_abs_diff(last_capacity, sample_capacity) >= BATTERY_CAPACITY_JUMP_GUARD) {
            TAL_PR_NOTICE("ignore abnormal battery drop: %u -> %u", last_capacity, sample_capacity);
            return FALSE;
        }

        last_capacity = sample_capacity;
    }

    return last_capacity != prev_capacity;
}

STATIC VOID __voltage_convert_task(VOID* arg)
{
    BOOL_T is_charging = FALSE;
    ai_battery_conf_t *bc = (ai_battery_conf_t *)arg;
    UINT8_T capacity = 0;
    BOOL_T capacity_changed = FALSE;

    // 前几次采样不准
    INT_T i = 0;
    for (i = 0; i < BATTERY_ADC_WARMUP_TIMES; i++) {
        capacity = __adc_conv_capacity(bc);
        tal_system_sleep(BATTERY_ADC_WARMUP_DELAY_MS);
    }

    // 首次上电，使用当前采样值作为初始电量
    // 非首次上电，即进出低功耗，初始化时候，保留上次采样值
    if (last_capacity == 0xff) {
        last_capacity = capacity;
    }

    // 初始化结束后，强制上报1次
    TAL_PR_DEBUG("force upload battery capacity, cap: %u", last_capacity);
    tuya_ai_battery_force_upload();

    is_running = TRUE;
    is_stop = FALSE;
    is_charging = bc->is_charging_cb && (bc->is_charging_cb() == TRUE);

    while (is_running) {
        BOOL_T charging_now = bc->is_charging_cb && (bc->is_charging_cb() == TRUE);

        if (charging_now != is_charging) {
            is_charging = charging_now;
            tuya_ai_battery_force_upload();
        }

        if (bc->mode == AI_BATTERY_MODE_ADC) {
            __adc_confidence(bc, &capacity);
            capacity_changed = __battery_capacity_apply(capacity, is_charging);
            if (capacity_changed) {
                PR_NOTICE("battery cap update: %u, charging=%d", last_capacity, is_charging);
                if (!is_charging) {
#ifdef ENABLE_TUYA_UI
                    tuya_ai_display_msg(&last_capacity, 1, TY_DISPLAY_TP_STAT_BATTERY);
#endif
                }
            }
        } else if (bc->mode == AI_BATTERY_MODE_IIC) {
            // TODO

        } else {
            PR_ERR("%s, mode not support", __func__);
        }

        tal_system_sleep(is_charging ? BATTERY_ADC_CHARGING_DELAY_MS : BATTERY_ADC_IDLE_DELAY_MS);
    }

    is_stop = TRUE;
    tal_thread_delete(__vol_conv_thread_handle);
}

VOID tuya_ai_battery_force_upload(VOID)
{
    if (sw_timer_id != NULL) {
        tal_sw_timer_trigger(sw_timer_id);
    }
}

UINT8_T tuya_ai_battery_get_capacity(VOID)
{
    tuya_ai_battery_force_upload();
    return last_report_capacity;
}

// 事件触发，主动上报电量
STATIC INT_T _event_active_cb(VOID_T *data)
{
    PR_NOTICE("active event");
    tuya_ai_battery_force_upload();
    return 0;
}
 
/**
 * @brief battery check init, creat battery monitor thread
 *
 * @param[in] conf, mode: how to get current battery capacity
 *                  cb: external action callback
 *
 * @return 0 is success, else is error.
 */
INT_T __tuya_ai_battery_init(ai_battery_conf_t *conf)
{
    OPERATE_RET rt = OPRT_OK;

    if (conf == NULL) {
        PR_ERR("%s, config is NULL", __func__);
        return -1;
    }

    if (conf->mode == AI_BATTERY_MODE_ADC) {
        // adc
        if (conf->adc.map == NULL) {
            PR_ERR("%s, voltage/cap map is NULL", __func__);
            return -1;
        }
        adc_chan = tkl_io_pin_to_func(conf->adc.adc_gpio, TUYA_IO_TYPE_ADC);
        if (adc_chan == OPRT_NOT_SUPPORTED) {
            PR_ERR("%s, adc pin is not support", __func__);
            return -1;
        }
        PR_NOTICE("%s, adc pin: %d", __func__, conf->adc.adc_gpio);

        TUYA_ADC_BASE_CFG_T cfg;
        UINT32_T ch_data = 0;
        ch_data |= BIT(adc_chan & 0xFF);

        cfg.ch_list.data = ch_data;
        cfg.ch_nums = 1;
        cfg.type = TUYA_ADC_INNER_SAMPLE_VOL;
        cfg.width = 12;
        cfg.mode = TUYA_ADC_CONTINUOUS;
        cfg.conv_cnt = ADC_CONV_TIMES;
        tkl_adc_init(0, &cfg);

#if defined(T5AI_BOARD_DESKTOP) || defined(T5AI_BOARD_DESKTOP)
        extern OPERATE_RET tkl_adc_ioctl(ADC_IOCTL_CMD_E cmd,  VOID *args);
        uint8_t adc_id = 14;
        tkl_adc_ioctl(ADC_DIV_RESIS_CLOSE, &adc_id);    // 0:关内阻(量程0-1.1),1:开内阻(量程0-3.0)
#endif

    } else if (conf->mode == AI_BATTERY_MODE_IIC) { 
        // i2c TODO
        if ((conf->i2c.init == NULL) || (conf->i2c.deinit == NULL) || (conf->i2c.get_capacity == NULL)) {
            PR_ERR("%s, i2c config error %p %p %p", __func__, conf->i2c.init, conf->i2c.deinit, conf->i2c.get_capacity);
            return -1;
        }

        // init i2c
        conf->i2c.init();
    } else {
        PR_ERR("%s, mode not support", __func__);
        return -2;
    }

    memcpy(&batt_conf, conf, sizeof(ai_battery_conf_t));

    // create monitor timer
    TUYA_CALL_ERR_GOTO(tal_sw_timer_create(__timer_cb, &batt_conf, &sw_timer_id), __EXIT);

    PR_DEBUG("battery monitor timer start");
    TUYA_CALL_ERR_LOG(tal_sw_timer_start(sw_timer_id, BATTERY_REPORT_PERIOD_MS, TAL_TIMER_CYCLE));
    ty_subscribe_event(EVENT_POST_ACTIVATE, "ai_toy", _event_active_cb, SUBSCRIBE_TYPE_ONETIME);

    THREAD_CFG_T thread_cfg = {
        .thrdname = "conv",
        .priority = THREAD_PRIO_2,
        .stackDepth = 1024 * 2,
        .psram_mode = 1,
    };
    TUYA_CALL_ERR_LOG(tal_thread_create_and_start(&__vol_conv_thread_handle, NULL, NULL, __voltage_convert_task, &batt_conf, &thread_cfg));
    return 0;

__EXIT:
    return -2;
}

/**
 * @brief battery check deinit
 *
 * @return none
 */
VOID __tuya_ai_battery_deinit(VOID)
{
    // if not running, return
    if (!is_running)
        return;

    ty_unsubscribe_event(EVENT_POST_ACTIVATE, "ai_toy", _event_active_cb);

    // stop thread
    is_running = FALSE;
    while (!is_stop) {
        tal_system_sleep(10);
    }
    TAL_PR_DEBUG("voltage convert thread exit");
    if (batt_conf.mode == AI_BATTERY_MODE_ADC) {
        tkl_adc_deinit(0);
    } else if (batt_conf.mode == AI_BATTERY_MODE_IIC) {
        batt_conf.i2c.deinit();
    }

    __battery_monitor_timer_stop(sw_timer_id);
    sw_timer_id = NULL;
    return;
}

BOOL_T __tuya_ai_toy_battery_is_charging(VOID)
{
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_NONE;
    tkl_gpio_read(TUYA_AI_TOY_CHARGE_PIN, &level);
    TAL_PR_DEBUG("battery_is_charging level: %d", level);

    return level == charge_check_level;
}


STATIC INT_T __tuya_ai_toy_battery_callback(UINT8_T current_capacity)
{
    BOOL_T is_charging = __tuya_ai_toy_battery_is_charging();
    BATTERY_CHAEGE_STATUS charge_status = __tuya_ai_toy_charge_status_get(is_charging, current_capacity);
    UINT8_T prev_dp2_report_capacity = last_dp2_report_capacity;
    INT_T prev_charge_status = last_report_charge_status;
    BOOL_T boot_ready = tuya_ai_toy_boot_report_ready();
    OPERATE_RET report_rt = OPRT_OK;

    TAL_PR_DEBUG("[%s], is_charging: %d, boot_ready: %d, cap: %u, status: %s(%d), prev_status: %d",
                 __func__, is_charging, boot_ready, current_capacity,
                 __battery_charge_status_str(charge_status), charge_status, prev_charge_status);
    if (s_battery_cb) {
        s_battery_cb(current_capacity <= 20, is_charging);
    }

#if defined(ENABLE_TUYA_UI) && (ENABLE_TUYA_UI == 1)
    if (charge_status != s_last_ui_charge_status) {
        if (charge_status == BATTERY_NO_CHARGE) {
            UINT8_T display_capacity = (current_capacity != 0xff) ? current_capacity : last_capacity;
            tuya_ai_display_msg(&display_capacity, 1, TY_DISPLAY_TP_STAT_BATTERY);
        } else {
            tuya_ai_display_msg(NULL, 0, TY_DISPLAY_TP_STAT_CHARGING);
        }
        s_last_ui_charge_status = charge_status;
    }
#endif

    if (!boot_ready) {
        if (current_capacity != 0xff) {
            last_report_capacity = current_capacity;
        }
        last_report_charge_status = charge_status;
        s_battery_report_baselined = FALSE;
        TAL_PR_DEBUG("%s, skip battery report before boot ready, status=%d cap=%d", __func__, charge_status, current_capacity);
        return 0;
    }

    if (!s_battery_report_baselined) {
        TY_OBJ_DP_S batt_cap_dp_info[2];
        UINT_T dp_cnt = 0;

        if (!is_charging && (current_capacity != 0xff)) {
            batt_cap_dp_info[dp_cnt].dpid = 2;
            batt_cap_dp_info[dp_cnt].type = PROP_VALUE;
            batt_cap_dp_info[dp_cnt].value.dp_value = current_capacity;
            batt_cap_dp_info[dp_cnt].time_stamp = 0;
            dp_cnt++;
        }

        batt_cap_dp_info[dp_cnt].dpid = 5;
        batt_cap_dp_info[dp_cnt].type = PROP_ENUM;
        batt_cap_dp_info[dp_cnt].value.dp_enum = charge_status;
        batt_cap_dp_info[dp_cnt].time_stamp = 0;
        dp_cnt++;

        TAL_PR_NOTICE("%s, initial battery report sync, status=%s(%d), cap=%u, dp_cnt=%u",
                      __func__, __battery_charge_status_str(charge_status), charge_status, current_capacity, dp_cnt);
        if (!is_charging && (current_capacity != 0xff)) {
            TAL_PR_NOTICE("%s, report dpid2=%u dpid5=%d", __func__, current_capacity, charge_status);
        } else {
            TAL_PR_NOTICE("%s, report dpid5=%d", __func__, charge_status);
        }
        report_rt = dev_report_dp_json_async_force(NULL, batt_cap_dp_info, dp_cnt);
        if (report_rt == OPRT_OK) {
            TAL_PR_NOTICE("%s, battery dp report success, dp_cnt=%u", __func__, dp_cnt);
        } else {
            TAL_PR_ERR("%s, battery dp report failed, rt=%d, dp_cnt=%u", __func__, report_rt, dp_cnt);
        }

        if (current_capacity != 0xff) {
            last_report_capacity = current_capacity;
            if (!is_charging) {
                last_dp2_report_capacity = current_capacity;
            }
        }
        last_report_charge_status = charge_status;
        s_battery_report_baselined = TRUE;
        return 0;
    }

    if (!is_charging) {
        BOOL_T capacity_changed = (current_capacity != 0xff) && (current_capacity != prev_dp2_report_capacity);
        BOOL_T charge_changed = (prev_charge_status != BATTERY_NO_CHARGE);

        if (!capacity_changed && !charge_changed) {
            return 0;
        }

        TAL_PR_NOTICE("%s, capacity: %d", __func__, current_capacity);
        TY_OBJ_DP_S batt_cap_dp_info[2];
        UINT_T dp_cnt = 0;

        if (capacity_changed) {
            batt_cap_dp_info[dp_cnt].dpid = 2;
            batt_cap_dp_info[dp_cnt].type = PROP_VALUE;
            batt_cap_dp_info[dp_cnt].value.dp_value = current_capacity;
            batt_cap_dp_info[dp_cnt].time_stamp = 0;
            dp_cnt++;
        }

        if (charge_changed) {
            batt_cap_dp_info[dp_cnt].dpid = 5;
            batt_cap_dp_info[dp_cnt].type = PROP_ENUM;
            batt_cap_dp_info[dp_cnt].value.dp_enum = BATTERY_NO_CHARGE;
            batt_cap_dp_info[dp_cnt].time_stamp = 0;
            dp_cnt++;
        }

        if (dp_cnt > 0) {
            if (capacity_changed && charge_changed) {
                TAL_PR_NOTICE("%s, report dpid2=%u dpid5=%d", __func__, current_capacity, BATTERY_NO_CHARGE);
            } else if (capacity_changed) {
                TAL_PR_NOTICE("%s, report dpid2=%u", __func__, current_capacity);
            } else if (charge_changed) {
                TAL_PR_NOTICE("%s, report dpid5=%d", __func__, BATTERY_NO_CHARGE);
            }
            report_rt = dev_report_dp_json_async_force(NULL, batt_cap_dp_info, dp_cnt);
            if (report_rt == OPRT_OK) {
                TAL_PR_NOTICE("%s, battery dp report success, dp_cnt=%u", __func__, dp_cnt);
            } else {
                TAL_PR_ERR("%s, battery dp report failed, rt=%d, dp_cnt=%u", __func__, report_rt, dp_cnt);
            }
        }

        if (current_capacity != 0xff) {
            last_report_capacity = current_capacity;
            if (capacity_changed) {
                last_dp2_report_capacity = current_capacity;
            }
        }
        last_report_charge_status = BATTERY_NO_CHARGE;
    } else {
        BOOL_T charge_changed = (charge_status != prev_charge_status);

        if (!charge_changed) {
            if (current_capacity != 0xff) {
                last_report_capacity = current_capacity;
            }
            return 0;
        }

        TAL_PR_NOTICE("%s, charge status change: %s(%d) -> %s(%d), capacity: %u",
                      __func__,
                      __battery_charge_status_str(prev_charge_status), prev_charge_status,
                      __battery_charge_status_str(charge_status), charge_status,
                      current_capacity);
        TY_OBJ_DP_S batt_cap_dp_info;
        batt_cap_dp_info.dpid = 5;
        batt_cap_dp_info.type = PROP_ENUM;
        batt_cap_dp_info.value.dp_enum = charge_status;
        batt_cap_dp_info.time_stamp = 0;

        TAL_PR_NOTICE("%s, report dpid5=%d", __func__, charge_status);
        report_rt = dev_report_dp_json_async_force(NULL, &batt_cap_dp_info, 1);
        if (report_rt == OPRT_OK) {
            TAL_PR_NOTICE("%s, battery dp report success, dp_cnt=1", __func__);
        } else {
            TAL_PR_ERR("%s, battery dp report failed, rt=%d, dp_cnt=1", __func__, report_rt);
        }
        if (current_capacity != 0xff) {
            last_report_capacity = current_capacity;
        }
        last_report_charge_status = charge_status;
    }

    return 0;
}

// 电量变化回调，外部调用，电量低于20%，或者充电状态变化都会回调
STATIC VOID _battery_cb(BOOL_T is_low, BOOL_T is_charging)
{
    TAL_PR_DEBUG("battery low = %d, charging = %d", is_low, is_charging);
    if (is_low && s_is_battery_low != is_low) {
        // low battery
        TAL_PR_NOTICE("low battery alert");
        s_is_battery_low = is_low;
        // ty_ai_event_post(s_ai_proc_handle, TY_AI_EVENT_BATTERY_LOW);
    }
}

UINT8_T tuya_ai_toy_capacity_value_get()
{
    return last_report_capacity;
}

BOOL_T tuya_ai_toy_charge_state_get()
{
    return __tuya_ai_toy_battery_is_charging();
}

VOID_T tuya_ai_toy_charge_level_set(TUYA_GPIO_LEVEL_E level)
{
    charge_check_level = level;
}

OPERATE_RET tuya_ai_toy_battery_init(VOID)
{
    OPERATE_RET rt = OPRT_OK;
    s_battery_cb = _battery_cb;
    s_battery_report_baselined = FALSE;
    last_dp2_report_capacity = 0xff;
    last_report_charge_status = -1;
    s_last_ui_charge_status = -1;

    ai_battery_conf_t conf = {
        .mode = AI_BATTERY_MODE_ADC,
        .adc = {
            .adc_gpio = TUYA_AI_TOY_BATTERY_CAP_PIN,
            .map = bvc_map,
            .map_size = tuya_ai_vol2cap_map_size(),
        },
        .cb = __tuya_ai_toy_battery_callback,
        .is_charging_cb = __tuya_ai_toy_battery_is_charging,
    };

    // 充电状态IO
    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.direct = TUYA_GPIO_INPUT;
    cfg.mode = TUYA_GPIO_FLOATING;
    tkl_gpio_init(TUYA_AI_TOY_CHARGE_PIN, &cfg);

    // battery monitor init
    TUYA_CALL_ERR_RETURN(__tuya_ai_battery_init(&conf));

    return rt;
}

OPERATE_RET tuya_ai_toy_battery_uninit(VOID)
{
    __tuya_ai_battery_deinit();
    return OPRT_OK;
}

#endif
