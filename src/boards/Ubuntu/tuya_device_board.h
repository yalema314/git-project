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
#define TUYA_AI_TOY_AUDIO_TRIGGER_PIN   TUYA_GPIO_NUM_MAX
#define TUYA_AI_TOY_SPK_EN_PIN          TUYA_GPIO_NUM_26
#define TUYA_AI_TOY_LED_PIN             TUYA_GPIO_NUM_MAX
#define TUYA_AI_TOY_NET_PIN             TUYA_GPIO_NUM_MAX

#if defined(USING_UART_AUDIO_INPUT) && (USING_UART_AUDIO_INPUT == 1)
#define UART_CODEC_VENDOR_ID                1                       // 0-GX8006, 1-CI1302
#define UART_CODEC_UART_PORT                TUYA_UART_NUM_0         // UART port of voice chip
#define UART_CODEC_BOOT_IO                  TUYA_GPIO_NUM_2         // BOOT IO of voice chip (used for GX8006)
#define UART_CODEC_POWER_IO                 TUYA_GPIO_NUM_3         // Power control IO of voice chip (high level = operational)
#define UART_CODEC_SPK_FLOWCTL_IO           TUYA_GPIO_NUM_32        // UART flow control IO for speaker
#define UART_CODEC_SPK_FLOWCTL_IO_LEVEL     TUYA_GPIO_LEVEL_LOW    // Active level of UART flow control IO for speaker
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