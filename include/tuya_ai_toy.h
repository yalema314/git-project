/**
 * @file tuya_ai_toy.h
 * @brief Tuya AI Toy module - public API and types.
 *
 * Declares configuration, state types and control interfaces for the AI toy
 * (wukong) application: init, DP processing, trigger mode and timers.
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

#ifndef __TUYA_AI_TOY_H__
#define __TUYA_AI_TOY_H__

#include "tuya_cloud_types.h"
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
#include "tuya_cloud_wifi_defs.h"
#endif
#include "tal_sw_timer.h"
#include "tal_workqueue.h"
#include "smart_frame.h"
#include "wukong_ai_mode.h"

/** Network status enumeration for AI toy. */
typedef enum {
    TY_AI_NET_STATUS_UNCFG = 0,
    TY_AI_NET_STATUS_DISCONNECTED,
    TY_AI_NET_STATUS_CONNECTED,
    TY_AI_NET_STATUS_CONNECTING,
    TY_AI_NET_STATUS_DISCONNECTING,
    TY_AI_NET_STATUS_MAX,
} TY_AI_NET_STATUS_E;

/** AI toy hardware and trigger configuration. */
typedef struct {
    TUYA_GPIO_NUM_E    spk_en_pin;        /**< Speaker enable GPIO. */
    TUYA_GPIO_NUM_E    led_pin;           /**< LED GPIO. */
    TUYA_GPIO_NUM_E    audio_trigger_pin; /**< Audio trigger GPIO. */
    TUYA_GPIO_NUM_E    net_pin;           /**< Provision key GPIO, long press enters provision. */
    AI_TRIGGER_MODE_E  trigger_mode;      /**< 0: hold, 1: one-shot, 2: free conversation. */
} TY_AI_TOY_CFG_T;

/** AI toy player state. */
typedef enum {
    AI_TOY_PLAYER_IDLE,
    AI_TOY_PLAYER_READY,
    AI_TOY_PLAYER_START,
    AI_TOY_PLAYER_STOP,
} TY_AI_TOY_PLAYER_STATE_T;

/** AI toy runtime context. */
typedef struct {
    TY_AI_TOY_CFG_T   cfg;
    BOOL_T            lp_stat;
    BOOL_T            wakeup_stat;
    UINT8_T           volume;             /**< Volume 0~100. */
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE == 1)
    GW_WIFI_NW_STAT_E nw_stat;            /**< Network status. */
#endif
    TIMER_ID          idle_timer;
    TIMER_ID          lowpower_timer;
    THREAD_HANDLE     toy_task;
} TY_AI_TOY_T;

/**
 * @brief Initialize the AI toy module.
 * @param cfg Configuration (GPIO and trigger mode). Must not be NULL.
 * @return OPERATE_RET
 */
OPERATE_RET tuya_ai_toy_init(TY_AI_TOY_CFG_T *cfg);

/**
 * @brief Process received DP (data point) payload.
 * @param dp Received DP object. Must not be NULL.
 */
VOID tuya_ai_toy_dp_process(CONST TY_RECV_OBJ_DP_S *dp);

/**
 * @brief Get current trigger mode.
 * @return Current AI_TRIGGER_MODE_E value.
 */
AI_TRIGGER_MODE_E tuya_ai_toy_trigger_mode_get(VOID);

/**
 * @brief Set trigger mode.
 * @param mode Desired trigger mode.
 */
VOID tuya_ai_toy_trigger_mode_set(AI_TRIGGER_MODE_E mode);

/**
 * @brief Enable or disable low-power timer.
 * @param enable TRUE to enable, FALSE to disable.
 */
VOID tuya_ai_toy_lowpower_timer_ctrl(BOOL_T enable);

/**
 * @brief Enable or disable idle timer.
 * @param enable TRUE to enable, FALSE to disable.
 */
VOID tuya_ai_toy_idle_timer_ctrl(BOOL_T enable);

/**
 * @brief Report wake-up DP state to cloud.
 * @param enable TRUE means the device is in wake/listen state; FALSE means it needs wake-up again.
 */
VOID tuya_ai_toy_wakeup_dp_report(BOOL_T enable);

/**
 * @brief Whether boot-time cloud reporting is ready for non-critical DP uploads.
 * @return TRUE when the device has finished the boot-time switch report, FALSE otherwise.
 */
BOOL_T tuya_ai_toy_boot_report_ready(VOID);

#endif /* __TUYA_AI_TOY_H__ */
