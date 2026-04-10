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
    
    // set countdown timer tool
    TUYA_CALL_ERR_GOTO(WUKONG_MCP_TOOL_ADD(
        "device_countdown_timer_set",
        "Set a countdown timer. Supports multiple time formats: seconds, minutes, hours\n"
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
    wukong_mcp_server_destroy();
    TAL_PR_DEBUG("MCP Server deinitialized successfully");
    return OPRT_OK;
}
