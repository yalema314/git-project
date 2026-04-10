/**
 * @file tuya_ai_toy_mcp.c
 * @brief Example implementation of MCP server callbacks for AI Toy device.
 * @version 0.1
 * @date 2025-11-01
 *
 * @copyright Copyright (c) 2025 Tuya Inc. All Rights Reserved.
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
#include <stdio.h>
#include "tuya_ai_toy.h"
#include "tuya_ai_agent.h"
#include "tuya_device_cfg.h"
#include "tal_log.h"
#include "wukong_ai_mcp_server.h"
#if defined(T5AI_BOARD_DESKTOP) && T5AI_BOARD_DESKTOP == 1
#include "tuya_motion_ctrl.h"
#endif
#if defined(ENABLE_TUYA_CAMERA) && ENABLE_TUYA_CAMERA == 1
#include "tuya_ai_toy_camera.h"
#endif
#include "skill_clock.h"

#ifndef APP_BIN_NAME
#define APP_BIN_NAME "tuyaos_demo_wukong_ai"
#endif
#ifndef USER_SW_VER
#define USER_SW_VER "1.0.0"
#endif

STATIC OPERATE_RET __get_device_info(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    ty_cJSON *json = NULL;

    json = ty_cJSON_CreateObject();
    if (!json) {
        TAL_PR_ERR("Create JSON object failed");
        return OPRT_MALLOC_FAILED;
    }

    // Implement device info retrieval logic here
    // Add device information
    ty_cJSON_AddStringToObject(json, "model", APP_BIN_NAME);
    ty_cJSON_AddStringToObject(json, "serialNumber", "123456789");
    ty_cJSON_AddStringToObject(json, "firmwareVersion", USER_SW_VER);

    // Set return value
    wukong_mcp_return_value_set_json(ret_val, json);
    return OPRT_OK;
}

#if defined(ENABLE_TUYA_CAMERA) && (ENABLE_TUYA_CAMERA == 1)
OPERATE_RET __take_photo(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    OPERATE_RET rt = OPRT_OK;
    BYTE_T *image_data = NULL;
    UINT_T image_size = 0;
    TUYA_CALL_ERR_LOG(tuya_ai_toy_camera_start());
    // 这里sleep 2000是用于将摄像头图形投送到屏幕，可以简单调整一下拍摄内容
    tal_system_sleep(3000);
    rt = tuya_ai_toy_camera_get_jpeg_frame(&image_data, &image_size, NULL);
    if (OPRT_OK != rt) {
        TAL_PR_ERR("get jpeg frame err, rt:%d", rt);
        return rt;
    }
    rt = wukong_mcp_return_value_set_image(ret_val, MCP_IMAGE_MIME_TYPE_JPEG, image_data, image_size);
    if (OPRT_OK != rt) {
        TAL_PR_ERR("set return image err, rt:%d", rt);
        tal_free(image_data);
        return rt;
    }
    tal_free(image_data);
    TUYA_CALL_ERR_LOG(tuya_ai_toy_camera_stop());
    return OPRT_OK;
}
#endif

// __set_volume
STATIC OPERATE_RET __set_volume(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T volume = 50; // default volume
    OPERATE_RET rt = OPRT_OK;

    TAL_PR_DEBUG("__set_volume enter");

    // Parse properties to get volume
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "volume") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            volume = prop->default_val.int_val;
            break;
        }
    }

    /* Reuse the AI toy volume API so MCP follows the same save/report behavior as keys and App DP. */
    TUYA_CALL_ERR_RETURN(tuya_ai_toy_volume_set((UINT8_T)volume));
    TAL_PR_DEBUG("set volume to %d", volume);

    // Set return value
    wukong_mcp_return_value_set_bool(ret_val, TRUE);

    TAL_PR_DEBUG("__set_volume exit");
    return OPRT_OK;
}

// __set_mode
STATIC OPERATE_RET __set_mode(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T mode = tuya_ai_toy_trigger_mode_get(); // default volume

    // Parse properties to get volume
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "mode") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            mode = prop->default_val.int_val;
            break;
        }
    }

    // Implement actual volume setting logic here
    wukong_ai_mode_switch_to(mode);
    TAL_PR_DEBUG("set mode to %d", mode);

    // Set return value
    wukong_mcp_return_value_set_bool(ret_val, TRUE);

    return OPRT_OK;
}

#if defined(ENABLE_APP_MCP_CLOCK_ALARM) && (ENABLE_APP_MCP_CLOCK_ALARM == 1)
// __set_alarm
STATIC OPERATE_RET __set_alarm(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    OPERATE_RET rt = OPRT_OK;
    INT_T operation = 0;
    CHAR_T *alarm_time = NULL;
    CHAR_T *detail_time = NULL;
    CHAR_T *label = NULL;
    UCHAR_T hours = 0;
    UCHAR_T minutes = 0;
    INT_T parsed_hours = 0;
    INT_T parsed_minutes = 0;
    TIME_T detail_time_posix = 0;
    INT_T repeat_sched_freq = 0;
    UCHAR_T mon_of_week = 0, tue_of_week = 0, wed_of_week = 0, thu_of_week = 0, fri_of_week = 0, sat_of_week = 0, sun_of_week = 0;
    BOOL_T snooze_enabled = 0;
    INT_T snooze_interval = 0;
    UCHAR_T snooze_count = 0;
    BOOL_T delete_all = FALSE;
    BOOL_T alarm_time_ready = FALSE;

    TAL_PR_DEBUG("__set_alarm enter");

    // Parse the MCP alarm tool payload into the local alarm structure used by skill_clock.
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "operation") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            operation = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "delete_all") == 0 && prop->type == MCP_PROPERTY_TYPE_BOOLEAN) {
            delete_all = prop->default_val.bool_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "alarm_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            alarm_time = prop->default_val.str_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "detail_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            detail_time = prop->default_val.str_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "label") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            label = prop->default_val.str_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "repeat_sched_freq") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            repeat_sched_freq = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "mon_of_week") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            mon_of_week = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "tue_of_week") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            tue_of_week = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "wed_of_week") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            wed_of_week = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "thu_of_week") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            thu_of_week = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "fri_of_week") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            fri_of_week = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "sat_of_week") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            sat_of_week = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "sun_of_week") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            sun_of_week = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "snooze_enabled") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            snooze_enabled = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "snooze_interval") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            snooze_interval = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "snooze_count") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            snooze_count = prop->default_val.int_val;
            break;
        }
    }

    // detail_time 支持秒级绝对时间，用于一次性短时提醒。
    if ((detail_time != NULL) && (strlen(detail_time) > 0)) {
        detail_time_posix = wukong_clock_time_mktime(detail_time);
        if (detail_time_posix > 0) {
            POSIX_TM_S detail_tm;

            memset(&detail_tm, 0, SIZEOF(detail_tm));
            tal_time_get_local_time_custom(detail_time_posix, &detail_tm);
            hours = (UCHAR_T)detail_tm.tm_hour;
            minutes = (UCHAR_T)detail_tm.tm_min;
            alarm_time_ready = TRUE;
        }
    }

    if ((alarm_time != NULL) && (strlen(alarm_time) > 0)) {
        if (sscanf(alarm_time, "%d:%d", &parsed_hours, &parsed_minutes) == 2) {
            hours = (UCHAR_T)parsed_hours;
            minutes = (UCHAR_T)parsed_minutes;
            alarm_time_ready = TRUE;
        } else {
            TAL_PR_ERR("parse alarm_time '%s' fail !", alarm_time);
            if (((operation == AI_CLOCK_ALARM_OPR_ADD) || (operation == AI_CLOCK_ALARM_OPR_UPDATE)) &&
                (repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE) &&
                (detail_time_posix > 0)) {
                rt = OPRT_OK;
            } else if ((operation == AI_CLOCK_ALARM_OPR_DELETE) && (delete_all == TRUE)) {
                rt = OPRT_OK;
            } else {
                rt = OPRT_INVALID_PARM;
            }
        }
    }

    if ((alarm_time_ready == FALSE) && (operation == AI_CLOCK_ALARM_OPR_DELETE) && (delete_all == TRUE)) {
        rt = OPRT_OK;
    } else if (alarm_time_ready == FALSE) {
        TAL_PR_ERR("alarm time info is missing");
        rt = OPRT_INVALID_PARM;
    }

    if (rt == OPRT_OK) {
        TY_AI_CLOCK_ALARM_CFG_T alarm = {
            .hours = hours,
            .minutes = minutes,
            .detail_time = detail_time_posix,
            .label = label,
            .repeat_sched_freq = (TY_AI_CLOCK_ALARM_REPEAT_E)repeat_sched_freq,
            .weekly_recurrence_day = ((mon_of_week << 1) | (tue_of_week << 2) | (wed_of_week << 3) |
                                      (thu_of_week << 4) | (fri_of_week << 5) | (sat_of_week << 6) |
                                      (sun_of_week << 0)),
            .snooze_enabled = snooze_enabled,
            .snooze_interval_sec = snooze_interval,
            .snooze_count = snooze_count
        };

        rt = wukong_clock_set_alarm((TY_AI_CLOCK_ALARM_OPR_TYPE_E)operation, delete_all, &alarm);
        if (rt == OPRT_OK) {
            wukong_mcp_return_value_set_bool(ret_val, TRUE);
        }
    }

    if (rt != OPRT_OK) {
        wukong_mcp_return_value_set_bool(ret_val, FALSE);
    }

    TAL_PR_DEBUG("__set_alarm exit");
    return rt;
}

// __set_alarm_query
STATIC OPERATE_RET __set_alarm_query(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T query_method = 0;
    CHAR_T *start_time = NULL;
    CHAR_T *end_time = NULL;
    CHAR_T *ret_content = NULL;

    TAL_PR_DEBUG("__set_alarm_query enter");

    // Return local alarm data so the LLM can answer voice follow-up questions without needing a screen page.
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "query_method") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            query_method = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "start_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            start_time = prop->default_val.str_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "end_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            end_time = prop->default_val.str_val;
            break;
        }
    }

    TY_AI_CLOCK_ALARM_QUERY_CFG_T alarm_query_info = {
        .start_time = wukong_clock_time_mktime(start_time),
        .end_time = wukong_clock_time_mktime(end_time)
    };
    ret_content = wukong_clock_set_alarm_query((TY_AI_CLOCK_ALARM_QUERY_METHOD_E)query_method, &alarm_query_info);
    if (ret_content != NULL) {
        wukong_mcp_return_value_set_str(ret_val, ret_content);
        tal_free(ret_content);
    } else {
        wukong_mcp_return_value_set_bool(ret_val, TRUE);
    }

    TAL_PR_DEBUG("__set_alarm_query exit");
    return OPRT_OK;
}
#endif

STATIC OPERATE_RET __set_countdown_timer(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T operation = 0;    // default operation type
    INT_T hours = 0;        // default hours
    INT_T minutes = 0;      // default minutes
    INT_T seconds = 0;      // default seconds

    TAL_PR_DEBUG("__set_countdown_timer enter");

    // Parse properties to get operation type(0=start, 1=pasue, 2=resume, 3=stop)
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "operation") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            operation = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get hours
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "hour_duration") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            hours = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get minutes
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "minute_duration") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            minutes = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get seconds
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "second_duration") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            seconds = prop->default_val.int_val;
            break;
        }
    }

    // Set return value
    if (wukong_clock_set_countdown_timer((TY_AI_CLOCK_TIMER_OPR_TYPE_E)operation, hours, minutes, seconds) == OPRT_OK) {
        wukong_mcp_return_value_set_bool(ret_val, TRUE);
    } else {
        wukong_mcp_return_value_set_bool(ret_val, FALSE);
    }
    
    TAL_PR_DEBUG("__set_countdown_timer exit");
    return OPRT_OK;
}

// __set_stopwatch_timer
STATIC OPERATE_RET __set_stopwatch_timer(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T operation = 0;   // default operation

    TAL_PR_DEBUG("__set_stopwatch_timer enter");

    // Parse properties to get operation type(0=start, 1=pasue, 2=resume, 3=stop, 4=reset)
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "operation") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            operation = prop->default_val.int_val;
            break;
        }
    }

    // Set return value
    if (wukong_clock_set_stopwatch_timer((TY_AI_CLOCK_TIMER_OPR_TYPE_E)operation) == OPRT_OK) {
        wukong_mcp_return_value_set_bool(ret_val, TRUE);
    } else {
        wukong_mcp_return_value_set_bool(ret_val, FALSE);
    }

    TAL_PR_DEBUG("__set_stopwatch_timer exit");
    return OPRT_OK;
}

// __set_pomodoro_timer
STATIC OPERATE_RET __set_pomodoro_timer(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T work_duration = 25;           // default work duration
    INT_T short_break_duration = 5;     // default short work duration
    INT_T long_break_duration = 15;     // default long work duration
    INT_T operation = 0;                // default operation

    TAL_PR_DEBUG("__set_pomodoro_timer enter");

    // Parse properties to get operation type(0=start, 1=pasue, 2=resume, 3=stop)
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "operation") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            operation = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get work_duration
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "work_duration") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            work_duration = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get short break duration
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "short_break_duration") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            short_break_duration = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get long break duration
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "long_break_duration") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            long_break_duration = prop->default_val.int_val;
            break;
        }
    }

    // Set return value
    TY_AI_CLOCK_POMODORO_TIMER_CFG_T pomodoro = {
        .work_duration = work_duration,
        .short_break_duration = short_break_duration,
        .long_break_duration = long_break_duration
    };
    if (wukong_clock_set_pomodoro_timer((TY_AI_CLOCK_TIMER_OPR_TYPE_E)operation, &pomodoro) == OPRT_OK) {
        wukong_mcp_return_value_set_bool(ret_val, TRUE);
    } else {
        wukong_mcp_return_value_set_bool(ret_val, FALSE);
    }   

    TAL_PR_DEBUG("__set_pomodoro_timer exit");
    return OPRT_OK;
}

// __set_schedule
STATIC OPERATE_RET __set_schedule(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T operation = 0;
    CHAR_T *start_time = NULL;
    CHAR_T *end_time = NULL;
    CHAR_T *location = NULL;
    CHAR_T *description = NULL;
    INT_T categories = 0;

    TAL_PR_DEBUG("__set_schedule enter");

    // Parse properties to get schedule operation type(0=add schedule, 1=delete schedule, 2=update schedule)
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "operation") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            operation = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get start time
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "start_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            start_time = prop->default_val.str_val;
            break;
        }
    }

    // Parse properties to get end time
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "end_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            end_time = prop->default_val.str_val;
            break;
        }
    }

    // Parse properties to get location
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "location") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            location = prop->default_val.str_val;
            break;
        }
    }

    // Parse properties to get description
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "description") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            description = prop->default_val.str_val;
            break;
        }
    }

    // Parse properties to get categories(0=meeting, 1=work, 2=personal, 3=health, 4=learning, 5=social, 6=other)
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "categories") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            categories = prop->default_val.int_val;
            break;
        }
    }

    // Set return value
    TY_AI_CLOCK_SCHED_CFG_T sched = {
        .start_time = wukong_clock_time_mktime(start_time),
        .end_time = wukong_clock_time_mktime(end_time),
        .location = location,
        .description = description,
        .categories = categories
    };

    if (wukong_clock_set_schedule((TY_AI_CLOCK_SCHED_OPR_TYPE_E)operation, &sched) == OPRT_OK) {
        wukong_mcp_return_value_set_bool(ret_val, TRUE);
    } else {
        wukong_mcp_return_value_set_bool(ret_val, FALSE);
    }

    TAL_PR_DEBUG("__set_schedule exit");
    return OPRT_OK;
}

// __set_schedule_query
STATIC OPERATE_RET __set_schedule_query(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    INT_T query_method = 0;
    CHAR_T *start_time = NULL;
    CHAR_T *end_time = NULL;
    INT_T categories = 0;
    CHAR_T *keyword = NULL;
    CHAR_T *ret_content = NULL;

    TAL_PR_DEBUG("__set_schedule_query enter");

    // Parse properties to get schedule query mothod(0=by time range, 1=by category, 2=by keyword)
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "query_method") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            query_method = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get query start time
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "start_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            start_time = prop->default_val.str_val;
            break;
        }
    }

    // Parse properties to get query end time
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "end_time") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            end_time = prop->default_val.str_val;
            break;
        }
    }

    // Parse properties to get categories filter(0=meeting, 1=work, 2=personal, 3=health, 4=learning, 5=social, 6=other)
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "categories") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            categories = prop->default_val.int_val;
            break;
        }
    }

    // Parse properties to get search keyword
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "keyword") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            keyword = prop->default_val.str_val;
            break;
        }
    }

    // Set return value
    TY_AI_CLOCK_SCHED_QUERY_CFG_T sched_query_info = {
        .categories = categories,
        .start_time = wukong_clock_time_mktime(start_time),
        .end_time = wukong_clock_time_mktime(end_time),
        .keyword = keyword
    };
    
    // query clock existed or not
    ret_content = wukong_clock_set_schedule_query((TY_AI_CLOCK_SCHED_QUERY_METHOD_E)query_method, &sched_query_info);
    if (ret_content != NULL) {
        wukong_mcp_return_value_set_str(ret_val, ret_content);
        tal_free(ret_content);
    } else {
        wukong_mcp_return_value_set_bool(ret_val, FALSE);
    }

    TAL_PR_DEBUG("__set_schedule_query exit");
    return OPRT_OK;
}

#if defined(T5AI_BOARD_DESKTOP) && T5AI_BOARD_DESKTOP == 1
STATIC OPERATE_RET __set_motion_control(CONST MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, VOID *user_data)
{
    TAL_PR_DEBUG("__set_motion_control enter");
    UINT_T mode = 0;
    UINT_T angle = 0;

    //0=left,1=right,2=appoint angle,3=clockwise,4=counterclockwise,5=point move,6=reset,7=stop
    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "motion_mode") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            TAL_PR_DEBUG("__set_motion_control motion_mode: %d", prop->default_val.int_val);
            mode = prop->default_val.int_val;
            break;
        }
    }

    for (INT_T i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "rotate_value") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            TAL_PR_DEBUG("__set_motion_control rotate_value: %d", prop->default_val.int_val);
            angle = prop->default_val.int_val;
            break;
        }
    }

    if(tuya_motion_send_msg(mode, angle) == OPRT_OK){
        wukong_mcp_return_value_set_bool(ret_val, TRUE);
    } else {
        wukong_mcp_return_value_set_bool(ret_val, FALSE);
    }
    
    TAL_PR_DEBUG("__set_motion_control exit");
    return OPRT_OK;
}
#endif

OPERATE_RET tuya_ai_toy_mcp_init(VOID)
{
    OPERATE_RET rt = OPRT_OK;

    TUYA_CALL_ERR_GOTO(wukong_clock_countdown_init(), err);

#if defined(ENABLE_APP_MCP_CLOCK_ALARM) && (ENABLE_APP_MCP_CLOCK_ALARM == 1)
    TUYA_CALL_ERR_GOTO(wukong_clock_alarm_init(), err);
#endif

    // FIXME: Set actual MCP server name and mcp version
    wukong_mcp_server_init("Tuya MCP Server", "1.0");

    // device info get tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_info_get",
        "Get device information such as model, serial number, and firmware version.",
        __get_device_info,
        NULL
    ), err);

#if defined(ENABLE_TUYA_CAMERA) && (ENABLE_TUYA_CAMERA == 1)
    // device camera take photo tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_camera_take_photo",
        "Activates the device's camera to capture one or more photos.\n"
        "Parameters:\n"
        "- count (int): Number of photos to capture (1-10).\n"
        "Response:\n"
        "- Returns the captured photos encoded in Base64 format.",
        __take_photo,
        NULL,
        MCP_PROP_STR("question", "The question prompting the photo capture."),
        MCP_PROP_INT_DEF_RANGE("count", "Number of photos to capture (1-10).", 1, 1, 10)
    ), err);
#endif

    // set volume tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_audio_volume_set",
        "Sets the device's volume level.\n"
        "Parameters:\n"
        "- volume (int): The volume level to set (0-100).\n"
        "Response:\n"
        "- Returns true if the volume was set successfully.",
        __set_volume,
        NULL,
        MCP_PROP_INT_RANGE("volume", "The volume level to set (0-100).", 0, 100)
    ), err);

    // set mode tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_audio_mode_set",
        "Set the device's audio interaction mode. This controls how the device listens for and responds to voice input.",
        __set_mode,
        NULL,
        MCP_PROP_INT_RANGE("mode", "The desired interaction mode: 'hold_to_talk' (长按对话), 'press_to_talk' (按键对话), 'wake_word' (唤醒模式), or 'free_conversation' (自由对话)", 0, 3)
    ), err);

#if defined(ENABLE_APP_MCP_CLOCK_ALARM) && (ENABLE_APP_MCP_CLOCK_ALARM == 1)
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_alarm_set",
        "Set an alarm that triggers at a specific clock time(absolute time like '9:00 AM') with ZERO DURATION. \n"
        "Supports repetition and snooze.\n"
        "TYPICAL USER PHRASES:\n"
        "- 'Wake me up at 7 AM'.\n"
        "- 'Wake me up at 8 am every Monday morning'.\n"
        "- 'Remind me tomorrow at 3 PM'.\n"
        "- 'Set an alarm for 8:00 every morning'.\n"
        "CRITICAL: \n"
        "- For one-time alarms, MUST provide detailed timestamp in ISO 8601 format as 'detail_time' parameter.\n"
        "- For short relative reminders like 'in 10 seconds' or 'in 5 minutes', prefer device_countdown_timer_set.\n"
        "- Use alarm when ONLY a start time is provided, especially with 'wake', 'remind', 'alert' verbs.\n"
        "Response:\n"
        "- Returns true if the alarm was set successfully.",
        __set_alarm,
        NULL,
        MCP_PROP_INT_RANGE("operation",             "Operation type(0=add alarm, 1=delete alarm, 2=update alarm)", 0, 2),
        MCP_PROP_STR_DEF("alarm_time",              "Absolute time for alarm trigger (format: 'HH:MM' 24-hour, e.g., '07:30')", ""),
        MCP_PROP_STR_DEF("detail_time",             "Absolute time for alarm trigger (ISO 8601 format). required when repeat_sched_freq is 'once', Example: '2024-01-15T14:30:00'", ""),
        MCP_PROP_STR_DEF("label",                   "Label for the alarm (e.g., 'Wake Up', 'turn_off_lights', 'send_email(optional)')", ""),
        MCP_PROP_INT_DEF_RANGE("repeat_sched_freq", "How often the alarm repeats. (0=once, 1=daily, 2=custom_weekly)", 0, 0, 2),
        MCP_PROP_INT_DEF_RANGE("mon_of_week",       "Selects monday as weekly recurrence days (required when repeat_sched_freq is 'custom_weekly', 0=unselected, 1=selected)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("tue_of_week",       "Selects tuesday as weekly recurrence days (required when repeat_sched_freq is 'custom_weekly', 0=unselected, 1=selected)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("wed_of_week",       "Selects wednesday as weekly recurrence days (required when repeat_sched_freq is 'custom_weekly', 0=unselected, 1=selected)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("thu_of_week",       "Selects thursday as weekly recurrence days (required when repeat_sched_freq is 'custom_weekly', 0=unselected, 1=selected)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("fri_of_week",       "Selects friday as weekly recurrence days (required when repeat_sched_freq is 'custom_weekly', 0=unselected, 1=selected)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("sat_of_week",       "Selects saturday as weekly recurrence days (required when repeat_sched_freq is 'custom_weekly', 0=unselected, 1=selected)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("sun_of_week",       "Selects sunday as weekly recurrence days (required when repeat_sched_freq is 'custom_weekly', 0=unselected, 1=selected)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("snooze_enabled",    "Whether snooze is enabled. (0=disable, 1=enable)", 0, 0, 1),
        MCP_PROP_INT_DEF_RANGE("snooze_interval",   "Interval between each snooze (only supports seconds, e.g., '300 seconds'. required if snooze_enabled is 'enable')", 0, 0, 3600),
        MCP_PROP_INT_DEF_RANGE("snooze_count",      "Maximum snooze count (alarm auto-dismisses after reaching this, required if snooze_enabled is 'enable')", 0, 0, 7),
        MCP_PROP_BOOL_DEF("delete_all",             "whether to delete all alarms.(required when operation is 'delete alarm', FALSE=delete specific alarm(s), TRUE=delete all alarms)", FALSE)
    ), err);

    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_alarm_query",
        "Query existing alarms. Filter by time range or select all. \n"
        "DO NOT USE when:\n"
        "- User clearly wants to CREATE/SET a new alarm (use device_alarm_set tool)\n"
        "- Request is straightforward creation without query intent\n"
        "- Phrases like 'set an alarm', 'wake me up' appear\n"
        "Response:\n"
        "- Returns true if the alarm query was set successfully.",
        __set_alarm_query,
        NULL,
        MCP_PROP_INT_DEF_RANGE("query_method",  "Query methods(0=by time range, 1=all)", 0, 0, 1),
        MCP_PROP_STR_DEF("start_time",          "Query start time (ISO 8601 format). Example: '2024-01-15T14:30:00'", ""),
        MCP_PROP_STR_DEF("end_time",            "Query end time (ISO 8601 format). Example: '2024-01-15T16:00:00'", "")
    ), err);
#endif
    
    // set countdown timer tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_countdown_timer_set",
        "Set a countdown timer. Supports multiple time formats: seconds, minutes, hours.\n"
        "Use this for short relative reminders like 'remind me in 10 seconds' or 'set a timer for 5 minutes'.\n"
        "Parameters:\n"
        "- hour_duration (int):     The Specified countdown hours duration (0-24).\n"
        "- minute_duration (int):   The Specified countdown minutes duration (0-60).\n"
        "- second_duration (int):   The Specified countdown seconds duration (0-60).\n"
        "- operation (int):         The action type (0=start, 1=pasue, 2=resume, 3=stop or exit).\n"
        "Response:\n"
        "- Returns true if the countdown timer was set successfully.",
        __set_countdown_timer,
        NULL,
        MCP_PROP_INT_RANGE("hour_duration",     "The Specified countdown hours duration (0-24)", 0, 24),
        MCP_PROP_INT_RANGE("minute_duration",   "The Specified countdown minutes duration (0-60)", 0, 60),
        MCP_PROP_INT_RANGE("second_duration",   "The Specified countdown seconds duration (0-60)", 0, 60),
        MCP_PROP_INT_RANGE("operation",         "The operation type (0=start, 1=pasue, 2=resume, 3=stop or exit)", 0, 3)
    ), err);

    // set stopwatch timer tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_stopwatch_timer_set",
        "Start a count-up timer (stopwatch) beginning from 0. Supports pausing, resuming, stopping and reset.\n"
        "Parameters:\n"
        "- operation (int): The action type (0=start, 1=pasue, 2=resume, 3=stop or exit, 4=reset).\n"
        "Response:\n"
        "- Returns true if the stopwatch was set successfully.",
        __set_stopwatch_timer,
        NULL,
        MCP_PROP_INT_RANGE("operation", "The operation type (0=start, 1=pasue, 2=resume, 3=stop or exit, 4=reset)", 0, 4)
    ), err);

    // set pomodoro timer tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_pomodoro_timer_set",
        "Complete Pomodoro technique implementation with customizable work/break cycles.\n"
        "Use when user mentions 'pomodoro', 'focus timer', 'work session', 'concentrate for X minutes'. \n"
        "When users provide incomplete information, the system automatically applies default values. \n"
        "Work session duration: Default 25 minutes. \n"
        "Short break duration: Default 5 minutes. \n"
        "Long break duration: Default 15 minutes. \n"
        "Parameters:\n"
        "- work_duration (int):        Work session duration in minutes (default 25, range 1-120).\n"
        "- short_break_duration (int): Short break duration in minutes (default 5, range 1-30).\n"
        "- long_break_duration (int):  Long break duration in minutes (default 15, range 5-60). Occurs after specified number of completed work sessions.\n"
        "- operation (int):            The operation type (0=start, 1=pasue, 2=resume, 3=stop or exit).\n"
        "Response:\n"
        "- Returns true if the pomodoro timer was set successfully.",
        __set_pomodoro_timer,
        NULL,
        MCP_PROP_INT_DEF_RANGE("work_duration",         "Work session duration in minutes (default 25, range 1-120)", 25, 1, 120),
        MCP_PROP_INT_DEF_RANGE("short_break_duration",  "Short break duration in minutes (default 5, range 1-30)", 5, 1, 30),
        MCP_PROP_INT_DEF_RANGE("long_break_duration",   "Long break duration in minutes (default 15, range 5-60)", 15, 5, 60),
        MCP_PROP_INT_RANGE("operation",             "The operation type (0=start, 1=pasue, 2=resume, 3=stop or exit)", 0, 3)
    ), err);

    // set schedule tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_schedule_set",
        "Create and manage personal or team schedules, excluding schedules querying.\n"
        "Parameters:\n"
        "- operation  (int): Operation type(0=add schedule, 1=delete schedule, 2=update schedule).\n"
        "- start_time (str): Start time (ISO 8601 format). Example: '2024-01-15T14:30:00'.\n"
        "- end_time   (str): End time (ISO 8601 format). Example: '2024-01-15T16:00:00'.\n"
        "- location   (str): Event location (physical venue or online meeting link).\n"
        "- description(str): Detailed event description.\n"
        "- categories (int): Schedule category (0=meeting, 1=work, 2=personal, 3=health, 4=learning, 5=social, 6=other).\n"
        "Response:\n"
        "- Returns true if the schedule was set successfully.",
        __set_schedule,
        NULL,
        MCP_PROP_INT_RANGE("operation", "Operation type(0=add schedule, 1=delete schedule, 2=update schedule)", 0, 2),
        MCP_PROP_INT_RANGE("categories","Schedule category (0=meeting, 1=work, 2=personal, 3=health, 4=learning, 5=social, 6=other)", 0, 6),
        MCP_PROP_STR("start_time",      "Start time (ISO 8601 format). Example: '2024-01-15T14:30:00'"),
        MCP_PROP_STR("end_time",        "End time (ISO 8601 format). Example: '2024-01-15T16:00:00'"),
        MCP_PROP_STR("location",        "Event location (physical venue or online meeting link)"),
        MCP_PROP_STR("description",     "Detailed event description")
    ), err);

    // set schedule query tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_schedule_query_set",
        "Query local schedule information and format results for return to the LLM. \n"
        "Supports multiple query methods: by time range, by category, keyword search, etc. \n"
        "Returns schedule data in structured formats for further LLM processing and analysis.\n"
        "Parameters:\n"
        "- query_method (int): Query methods(0=by time range, 1=by category, 2=by keyword).\n"
        "- start_time (str):   Query start time (ISO 8601 format). Example: '2024-01-15T14:30:00'.\n"
        "- end_time (str):     Query end time (ISO 8601 format). Example: '2024-01-15T16:00:00'.\n"
        "- categories (int):   Category filter (category queries) (0=meeting, 1=work, 2=personal, 3=health, 4=learning, 5=social, 6=other).\n"
        "- keyword (str):      Search keyword.\n"
        "Response:\n"
        "- Returns true if the schedule was set successfully.",
        __set_schedule_query,
        NULL,
        MCP_PROP_INT_DEF_RANGE("query_method",  "Query methods(0=by time range, 1=by category, 2=by keyword)", 0, 0, 2),
        MCP_PROP_STR_DEF("start_time",          "Query start time (ISO 8601 format). Example: '2024-01-15T14:30:00'", ""),
        MCP_PROP_STR_DEF("end_time",            "Query end time (ISO 8601 format). Example: '2024-01-15T16:00:00'", ""),
        MCP_PROP_INT_DEF_RANGE("categories",    "Category filter (category queries) (0=meeting, 1=work, 2=personal, 3=health, 4=learning, 5=social, 6=other)", 6, 0, 6),
        MCP_PROP_STR_DEF("keyword",             "Search keyword", "")
    ), err);
#if defined(T5AI_BOARD_DESKTOP) && T5AI_BOARD_DESKTOP == 1
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_motion_control_set",
        "Motion rotation control, support left/right rotation, clockwise/counterclockwise rotation, point move, reset to zero and stop\n"
        "Parameters:\n"
        "- motion_mode (int):       Motion control mode (0=left rotate,1=right rotate,2=appoint angle,3=clockwise,4=counterclockwise,5=point move,6=reset).\n"
        "- rotate_value (int):      Rotation value (0-3600), unit: degree when value≤360, circle when value>360, invalid for point move and reset.\n"
        "Response:\n"
        "- Returns true if the motion control was set successfully.",
        __set_motion_control,
        NULL,
        MCP_PROP_INT_RANGE("motion_mode",       "Motion mode (0=left,1=right,2=appoint angle,3=clockwise,4=counterclockwise,5=point move,6=reset,7=stop)", 0, 7),
        MCP_PROP_INT_RANGE("rotate_value",      "Rotation value (0-3600, ≤360=degree, >360=circle)", 0, 3600
        )
    ), err);
#endif


    TAL_PR_DEBUG("MCP Server initialized successfully");
    return OPRT_OK;

err:
    // destroy MCP server on failure
    wukong_mcp_server_destroy();
    return rt;
}

OPERATE_RET tuya_ai_toy_mcp_deinit(VOID)
{
    wukong_clock_countdown_deinit();
#if defined(ENABLE_APP_MCP_CLOCK_ALARM) && (ENABLE_APP_MCP_CLOCK_ALARM == 1)
    wukong_clock_alarm_deinit();
#endif
    wukong_mcp_server_destroy();
    TAL_PR_DEBUG("MCP Server deinitialized successfully");
    return OPRT_OK;
}
