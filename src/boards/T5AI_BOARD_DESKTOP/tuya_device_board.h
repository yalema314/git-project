/**
 * @file tuya_device_board.h
 * @brief
 * @version 0.1
 * @date 2023-06-26
 *
 * @copyright Copyright (c) 2023 Tuya Inc. All Rights Reserved.
 *
 * Permission is hereby granted, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), Under the premise of complying
 * with the license of the third-party open source software contained in the software,
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software.
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 */

#ifndef __TUYA_DEVICE_BOARD_H__
#define __TUYA_DEVICE_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "tuya_app_config.h"
// default battery
#define TUYA_AI_TOY_BATTERY_ENABLE 1
#define TUYA_AI_TOY_CHARGE_PIN          TUYA_GPIO_NUM_2
#define TUYA_AI_TOY_BATTERY_CAP_PIN     TUYA_GPIO_NUM_12

// default pins
#define TUYA_AI_TOY_AUDIO_TRIGGER_PIN   TUYA_GPIO_NUM_28
#define TUYA_AI_TOY_SPK_EN_PIN          TUYA_GPIO_NUM_26
#define TUYA_AI_TOY_LED_PIN             TUYA_GPIO_NUM_MAX
#define TUYA_AI_TOY_NET_PIN             TUYA_GPIO_NUM_MAX


// device power onoff pin
#define DEVICE_POWER_NET_KEY_PIN     TUYA_GPIO_NUM_3
#define DEVICE_POWER_PIN             TUYA_GPIO_NUM_4

// default display
#define TUYA_LCD_IC_NAME     "spi_st7789v2"
#define TUYA_LCD_WIDTH        320
#define TUYA_LCD_HEIGHT       240
#define TUYA_LCD_ROTATION     TKL_DISP_ROTATION_0
#define LCD_FPS          15

// enable ai opus encode
#define ENABLE_APP_OPUS_ENCODER 1
#define ENABLE_APP_SPEEX_ENCODER 0

OPERATE_RET tuya_device_board_init();
#ifdef __cplusplus
}
#endif
#endif