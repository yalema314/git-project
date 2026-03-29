/**
 * @file tuya_device_board.c
 * @author www.tuya.com
 * @brief tuya_device_board module is used to
 * @version 0.1
 * @date 2022-10-28
 *
 * @copyright Copyright (c) tuya.inc 2022
 *
 */
#include "tuya_device_board.h"
#include "tuya_cloud_types.h"
#include "tal_log.h"
#include "tuya_key.h"

#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
#include "tuya_cloud_wifi_defs.h"
#endif

#if defined(TUYA_AI_TOY_BATTERY_ENABLE) && (TUYA_AI_TOY_BATTERY_ENABLE == 1)
#include "tuya_ai_battery.h"
#endif

/***********************************************************
************************macro define************************
***********************************************************/
#define LONG_KEY_TIME                   400
#define SEQ_KEY_TIME                    200

/***********************************************************
************************function define************************
***********************************************************/
STATIC VOID __on_ai_toy_desktop_device_off(UINT_T port, PUSH_KEY_TYPE_E type, INT_T cnt)
{
    switch (type) 
    {
        case LONG_KEY: 
        {
            TAL_PR_NOTICE("ai toy -> desktop device off");
            tkl_gpio_write(DEVICE_POWER_PIN, 0);
        }
        break;

        case NORMAL_KEY:
        {

        }
        break;

        case SEQ_KEY:
        {
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
            TAL_PR_NOTICE("ai toy -> desktop reset");
            tuya_iot_wf_gw_fast_unactive(GWCM_OLD, WF_START_SMART_AP_CONCURRENT);
#endif
        }
        break;

        case RELEASE_KEY:
        default:
            break;
    }
    return;    
}

STATIC OPERATE_RET __desktop_key_init(VOID)
{
    OPERATE_RET rt = OPRT_OK;

    // 上电瞬间锁住上电管脚电平
    TUYA_GPIO_BASE_CFG_T device_on;
    device_on.direct = TUYA_GPIO_OUTPUT,
    device_on.mode = TUYA_GPIO_PULLUP,
    device_on.level = TUYA_GPIO_LEVEL_HIGH,
    tkl_gpio_init(DEVICE_POWER_PIN, &device_on);
    tkl_gpio_write(DEVICE_POWER_PIN, 1);

    // 初始化关机按键 长按5s关机
    TUYA_CALL_ERR_RETURN(tuya_ai_toy_key_init(DEVICE_POWER_NET_KEY_PIN, TRUE, SEQ_KEY_TIME, 3*1000, __on_ai_toy_desktop_device_off));

    // TUYA_CALL_ERR_RETURN(tuya_imu_init(TUYA_I2C_NUM_0, TUYA_GPIO_NUM_22));
    return rt;
}

STATIC VOID __desktop_io_init(VOID)
{
    // SDIO
    tkl_io_pinmux_config(TUYA_IO_PIN_14, TUYA_SDIO_CLK);
    tkl_io_pinmux_config(TUYA_IO_PIN_15, TUYA_SDIO_CMD);
    tkl_io_pinmux_config(TUYA_IO_PIN_16, TUYA_SDIO_DATA0);

    // I2C0
    tkl_io_pinmux_config(TUYA_IO_PIN_20, TUYA_IIC0_SCL);
    tkl_io_pinmux_config(TUYA_IO_PIN_21, TUYA_IIC0_SDA);

    // SPI0
    tkl_io_pinmux_config(TUYA_IO_PIN_44, TUYA_SPI0_CLK);
    tkl_io_pinmux_config(TUYA_IO_PIN_45, TUYA_SPI0_CS);
    tkl_io_pinmux_config(TUYA_IO_PIN_46, TUYA_SPI0_MOSI);
    tkl_io_pinmux_config(TUYA_IO_PIN_47, TUYA_SPI0_MISO);   
    
    //UART2
    tkl_io_pinmux_config(TUYA_IO_PIN_40, TUYA_UART2_RX);
    tkl_io_pinmux_config(TUYA_IO_PIN_41, TUYA_UART2_TX);
}

/**
 * @brief evb board initialization
 *
 * @param[in] none
 *
 * @return OPRT_OK on success. Others on error, please refer to "tuya_error_code.h".
 */
OPERATE_RET tuya_device_board_init(VOID_T)
{
    OPERATE_RET rt = OPRT_OK;

    TAL_PR_NOTICE("ai toy -> init desktop");
    __desktop_io_init();
    __desktop_key_init();
    
    // motion
    TUYA_CALL_ERR_RETURN(tuya_motion_ctrl_init());

    tuya_ai_toy_charge_level_set(TUYA_GPIO_LEVEL_LOW);

    return rt;
}
