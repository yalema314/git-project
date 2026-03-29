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
// default pins
#define TUYA_AI_TOY_AUDIO_TRIGGER_PIN   TUYA_GPIO_NUM_12
#define TUYA_AI_TOY_SPK_EN_PIN          TUYA_GPIO_NUM_28
#define TUYA_AI_TOY_LED_PIN             TUYA_GPIO_NUM_MAX
#define TUYA_AI_TOY_NET_PIN             TUYA_GPIO_NUM_MAX

// default camera
#if defined(ENABLE_TUYA_CAMERA) && ENABLE_TUYA_CAMERA == 1
#define TUYA_AI_TOY_CAMERA_TYPE  TKL_VI_CAMERA_TYPE_DVP
#define TUYA_AI_TOY_CAMERA_FMT   TKL_CODEC_VIDEO_MJPEG
#define TUYA_AI_TOY_POWER_PIN    TUYA_GPIO_NUM_51
#define TUYA_AI_TOY_ACTV_LEVEL   TUYA_GPIO_LEVEL_HIGH
#define TUYA_AI_TOY_I2C_CLK      TUYA_GPIO_NUM_13
#define TUYA_AI_TOY_I2C_SDA      TUYA_GPIO_NUM_15
#define TUYA_AI_TOY_ISP_WIDTH    480
#define TUYA_AI_TOY_ISP_HEIGHT   480
#define TUYA_AI_TOY_ISP_FPS      10
#endif

// default display
#define TUYA_LCD_IC_NAME     "spi_st7735s"
#define TUYA_LCD_WIDTH        128
#define TUYA_LCD_HEIGHT       128
#define TUYA_LCD_ROTATION     TUYA_SCREEN_ROTATION_0
#define LCD_FPS          10

// enable ai opus encode
#define ENABLE_APP_OPUS_ENCODER 1
#define ENABLE_APP_SPEEX_ENCODER 0

OPERATE_RET tuya_device_board_init();
#ifdef __cplusplus
}
#endif
#endif