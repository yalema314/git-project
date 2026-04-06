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
#define TUYA_AI_TOY_BATTERY_ENABLE      1
#define TUYA_AI_TOY_CHARGE_PIN          TUYA_GPIO_NUM_21
#define TUYA_AI_TOY_BATTERY_CAP_PIN     TUYA_GPIO_NUM_28

// default pins
#define TUYA_AI_TOY_AUDIO_TRIGGER_PIN   TUYA_GPIO_NUM_19
#define TUYA_AI_TOY_DEEPSLEEP_WAKE_PIN  TUYA_GPIO_NUM_45
#define TUYA_AI_TOY_SPK_EN_PIN          TUYA_GPIO_NUM_20
#define TUYA_AI_TOY_LED_PIN             TUYA_GPIO_NUM_MAX
#define TUYA_AI_TOY_NET_PIN             TUYA_GPIO_NUM_18
#define TUYA_AI_TOY_MIC_GAIN_DEFAULT    65

// gc9a01 spi display pins
#define TUYA_LCD_SPI_CLK_PIN            TUYA_GPIO_NUM_14
#define TUYA_LCD_SPI_CS_PIN             TUYA_GPIO_NUM_15
#define TUYA_LCD_SPI_MOSI_PIN           TUYA_GPIO_NUM_16
#define TUYA_LCD_DC_PIN                 TUYA_GPIO_NUM_9
#define TUYA_LCD_RESET_PIN              TUYA_GPIO_NUM_17
#define TUYA_LCD_BL_PIN                 TUYA_GPIO_NUM_31
#define TUYA_LCD_POWER_CTRL_PIN         TUYA_GPIO_NUM_MAX

// default camera
#if defined(ENABLE_TUYA_CAMERA) && ENABLE_TUYA_CAMERA == 1
/* Update camera pins if your board also uses a camera; LCD CS now uses GPIO15. */
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
#define TUYA_LCD_IC_NAME     "spi_gc9a01"
#define TUYA_LCD_WIDTH        240
#define TUYA_LCD_HEIGHT       240
#define TUYA_LCD_ROTATION     TUYA_SCREEN_ROTATION_0
#define LCD_FPS               15
#define SUPERT_USE_EYES_UI    1

// enable ai opus encode
#define ENABLE_APP_OPUS_ENCODER 1
#define ENABLE_APP_SPEEX_ENCODER 0

#if defined(USING_UART_AUDIO_INPUT) && (USING_UART_AUDIO_INPUT == 1)
#define UART_CODEC_VENDOR_ID                1                       // 0-GX8006, 1-CI1302
#define UART_CODEC_UART_PORT                TUYA_UART_NUM_2         // UART port of voice chip
#define UART_CODEC_BOOT_IO                  TUYA_GPIO_NUM_2         // BOOT IO of voice chip (used for GX8006)
#define UART_CODEC_POWER_IO                 TUYA_GPIO_NUM_3         // Power control IO of voice chip (high level = operational)
#define UART_CODEC_SPK_FLOWCTL_IO           TUYA_GPIO_NUM_32        // UART flow control IO for speaker
#define UART_CODEC_SPK_FLOWCTL_IO_LEVEL     TUYA_GPIO_LEVEL_HIGH    // Active level of UART flow control IO for speaker
#define UART_CODEC_MUTE_IO_LEVEL            TUYA_GPIO_LEVEL_LOW     // Active level of mute IO
#define UART_CODEC_UPLOAD_FORMAT            1                       // 1-SPEEX, 2-OPUS

#define ENABLE_APP_OPUS_ENCODER 0
#define ENABLE_APP_SPEEX_ENCODER 1
#endif


OPERATE_RET tuya_device_board_init();

#ifdef __cplusplus
}
#endif
#endif
