#include "tal_system.h"
#include "tuya_uf_db.h"
#include "tal_sw_timer.h"
#include "base_event.h"
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
#include "tuya_iot_wifi_api.h"
#endif
#include "tal_log.h"
#include "tuya_key.h"
#include "tkl_gpio.h"
#include "tuya_device_cfg.h"

#define RESET_NETCNT_NAME     "rst_cnt"
#define RESET_NETCNT_MAX      3

INT_T __reset_count_read(UINT8_T *count)
{
    INT_T      rt = OPRT_OK;
    uFILE   *fp = NULL;
    INT_T     cnt = 0;

    fp = ufopen(RESET_NETCNT_NAME, "r+");
    if(NULL == fp) {
        TAL_PR_ERR("uf file %s can't open and read data!", RESET_NETCNT_NAME);
        return OPRT_NOT_EXIST;
    }

    TAL_PR_DEBUG("uf open OK");
    cnt = ufread(fp, count, 1);
    TAL_PR_DEBUG("uf file %s read data %d!", RESET_NETCNT_NAME, *count);

    rt = ufclose(fp);
    if(rt != OPRT_OK) {
        TAL_PR_ERR("uf file %s close error!", RESET_NETCNT_NAME);
        return rt;
    }
    
    return rt;
}


INT_T __reset_count_write(UINT8_T count)
{
    INT_T      rt = OPRT_OK;
    uFILE   *fp = NULL;
    INT_T     cnt = 0;

    fp = ufopen(RESET_NETCNT_NAME, "a+");
    if(NULL == fp) {
        TAL_PR_ERR("uf file %s can't open and read data!", RESET_NETCNT_NAME);
        return OPRT_NOT_EXIST;
    }

    if (0 != ufseek(fp, 0, UF_SEEK_SET)) {
        ufclose(fp);
        TAL_PR_ERR("uf file %s Set file offset to 0 error!", RESET_NETCNT_NAME);
        return OPRT_NOT_EXIST;
    }

    cnt = ufwrite(fp, &count, 1);
    if(cnt != 1) {
        TAL_PR_ERR("uf file %s write data error!", RESET_NETCNT_NAME);
    }

    rt = ufclose(fp);
    if(rt != OPRT_OK) {
        TAL_PR_ERR("uf file %s close error!", RESET_NETCNT_NAME);
        return rt;
    }

    return rt;
}


STATIC VOID __reset_netconfig_timer(TIMER_ID timer_id, VOID_T *arg)
{
    __reset_count_write(0);
    TAL_PR_DEBUG("reset cnt clear!");
}

INT_T __reset_netconfig_check(void *args)
{
    INT_T rt;
    UINT8_T rstcnt = 0;

    TUYA_CALL_ERR_LOG(__reset_count_read(&rstcnt));
    if (rstcnt < RESET_NETCNT_MAX) {
        return OPRT_OK;
    }

    __reset_count_write(0);
    TAL_PR_DEBUG("Reset ctrl data!");
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
    tuya_iot_wf_gw_fast_unactive(GWCM_OLD, WF_START_SMART_AP_CONCURRENT);
#endif

    return OPRT_OK;
}


INT_T __reset_netconfig_start(VOID* data)
{
    INT_T rt = OPRT_OK;
    UINT8_T rstcnt = 0;

    // ty_subscribe_event
    TAL_PR_NOTICE("ai toy -> power up/down %d times reset counter start", RESET_NETCNT_MAX);
    ty_subscribe_event(EVENT_SDK_DB_INIT_OK, "reset", __reset_netconfig_check, SUBSCRIBE_TYPE_ONETIME);

    // TUYA_RESET_REASON_E reason = tal_system_get_reset_reason(NULL);
    // TAL_PR_DEBUG("reset info -> reason is %d>>>", reason);
    // if(TUYA_RESET_REASON_POWERON != reason) {
    //     return OPRT_OK;
    // }
    TUYA_CALL_ERR_LOG(__reset_count_read(&rstcnt));
    TUYA_CALL_ERR_LOG(__reset_count_write(++rstcnt));

    TAL_PR_DEBUG("start reset cnt clear timer!!!!!");
    TIMER_ID rst_config_timer;
    tal_sw_timer_create(__reset_netconfig_timer, NULL, &rst_config_timer);
    tal_sw_timer_start(rst_config_timer, 5000, TAL_TIMER_ONCE);

    return OPRT_OK;
}

#if defined(WUKONG_BOARD_UBUNTU) && (WUKONG_BOARD_UBUNTU == 1) && defined(USING_UART_BOARD_INPUT) && (USING_UART_BOARD_INPUT == 1)
#include <stdio.h>
#include "wukong_ai_mode.h"

STATIC THREAD_HANDLE ubuntu_key_thread = NULL;

STATIC VOID __ubuntu_key_cb(PVOID_T args)
{
    PUSH_KEY_TYPE_E type = NORMAL_KEY;

    while(TRUE) {
        CHAR_T c = getchar();
        switch(c) {
            case 'A':
                if(type != LONG_KEY) {
                    type = LONG_KEY;
                } else {
                    type = RELEASE_KEY;
                }
                wukong_ai_handle_key(&type, 0);
                break;
            default:
                TAL_PR_NOTICE("-----------------type [A] to %s input", (type != LONG_KEY) ? "start" : "stop");
                break;
        }
    }
}

STATIC VOID __ubuntu_key_init(VOID)
{
    if(ubuntu_key_thread) {
        return;
    }

    THREAD_CFG_T cfg = {
        .priority = THREAD_PRIO_3,
        .stackDepth = 2*1024,
        .thrdname = "ubuntu_key"
    };

    tal_thread_create_and_start(&ubuntu_key_thread, NULL, NULL, __ubuntu_key_cb, NULL, &cfg);
}
#endif

OPERATE_RET tuya_reset_netconfig_init(VOID)
{
    ty_subscribe_event(EVENT_SDK_EARLY_INIT_OK, "early_init", __reset_netconfig_start, SUBSCRIBE_TYPE_ONETIME);
    return OPRT_OK;
}

OPERATE_RET tuya_ai_toy_key_init(TUYA_GPIO_NUM_E pin, BOOL_T low_detect, UINT32_T seqk_time_ms, UINT32_T longk_time_ms, KEY_CALLBACK cb)
{
#if defined(WUKONG_BOARD_UBUNTU) && (WUKONG_BOARD_UBUNTU == 1) && defined(USING_UART_BOARD_INPUT) && (USING_UART_BOARD_INPUT == 1)
    __ubuntu_key_init();
    return OPRT_OK;
#else
    if (TUYA_GPIO_NUM_MAX == pin) {
        return OPRT_OK;
    }

    OPERATE_RET rt = OPRT_OK;
    TAL_PR_DEBUG("ai toy -> init key %d", pin);
    TUYA_GPIO_BASE_CFG_T key_cfg = {
        .mode = TUYA_GPIO_PULLUP,
        .direct = TUYA_GPIO_INPUT,
        .level = TUYA_GPIO_LEVEL_HIGH
    };

    // audio trigger pin
    KEY_USER_DEF_S pin_cfg;
    pin_cfg.port                = pin;
    pin_cfg.low_level_detect    = low_detect;
    pin_cfg.lp_tp               = LP_ONCE_TRIG;
    pin_cfg.long_key_time       = longk_time_ms;
    pin_cfg.seq_key_detect_time = seqk_time_ms;
    pin_cfg.call_back           = cb;
    key_init(NULL, 0, 20);
    reg_proc_key(&pin_cfg);
    TUYA_CALL_ERR_LOG(tkl_gpio_init(pin , &key_cfg));
    TUYA_CALL_ERR_LOG(tkl_gpio_irq_enable(pin));
    TAL_PR_DEBUG("ai toy key %d seqk time %d, longk time %d", pin, seqk_time_ms, longk_time_ms);
    return rt;
#endif
}
