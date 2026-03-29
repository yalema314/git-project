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

STATIC VOID __t5ai_board_pinmux_init(VOID_T)
{
    tkl_io_pinmux_config(TUYA_IO_PIN_14, TUYA_SPI0_CLK);
    tkl_io_pinmux_config(TUYA_IO_PIN_15, TUYA_SPI0_CS);
    tkl_io_pinmux_config(TUYA_IO_PIN_16, TUYA_SPI0_MOSI);
}


/***********************************************************
************************macro define************************
***********************************************************/

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
    TAL_PR_NOTICE("ai toy -> init SuperT board");
    __t5ai_board_pinmux_init();
    return rt;
}
