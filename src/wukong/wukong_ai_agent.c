/**
 * @file ai_demo.c
 * @brief 
 * @version 0.1
 * @date 2025-03-21
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

#include "wukong_ai_agent.h"
#include "wukong_ai_skills.h"
#include "wukong_audio_input.h"
#include "wukong_audio_player.h"
#include "uni_log.h"
#include "base_event.h"
#include "tuya_device_cfg.h"
#include "tuya_svc_devos.h"
#include "tuya_ai_agent.h"
#include "tuya_ai_output.h"
#include <stdio.h>

STATIC WUKONG_EVENT_OUTPUT __s_event_notify_cb = NULL;
STATIC UINT16_T __s_audio_codec_type = AI_AUDIO_CODEC_MP3;
OPERATE_RET __wukong_ai_agent_event_cb(AI_EVENT_TYPE type, AI_PACKET_PT ptype, AI_EVENT_ID eid)
{
    PR_DEBUG("wukong ai agent -> recv event type: %d", type);

    if (AI_EVENT_START == type) {
        if (AI_PT_AUDIO == ptype) {
            //! start wukong audio player
            // wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TTS_START, __s_audio_codec_type, (CHAR_T*)eid, strlen(eid));
            wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TTS_START, __s_audio_codec_type, NULL, 0);
        }
    } else if ((AI_EVENT_CHAT_BREAK == type)) {
        //! cloud break, stop wukong audio player
        wukong_audio_player_stop(AI_PLAYER_FG);
        wukong_skill_notify_chat_break();
        wukong_ai_event_notify(WUKONG_AI_EVENT_CHAT_BREAK, NULL);
    } else if ((AI_EVENT_SERVER_VAD == type)) {
        //! cloud vad, just notify
        wukong_ai_event_notify(WUKONG_AI_EVENT_SERVER_VAD, NULL);
    } else if ((AI_EVENT_END == type)) {
        if (AI_PT_AUDIO == ptype) {
            //! stop wukong audio player
            // wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TTS_STOP, __s_audio_codec_type, (CHAR_T*)eid, strlen(eid));
            wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TTS_STOP, __s_audio_codec_type, NULL, 0);
        }
    } else if (AI_EVENT_CHAT_EXIT == type) {
        //! stop wukong audio player
        wukong_ai_event_notify(WUKONG_AI_EVENT_EXIT, NULL);
    }

    return OPRT_OK;
}

/** recv media attr */
OPERATE_RET __wukong_ai_agent_media_attr_cb(AI_BIZ_ATTR_INFO_T *attr)
{
    // PR_DEBUG("wukong ai agent -> recv media attr type: %d", attr->type);
    if (attr->type == AI_PT_AUDIO && attr->flag & AI_HAS_ATTR) {
        PR_DEBUG("wukong ai agent -> audio codec type: %d", attr->value.audio.base.codec_type);
        switch (attr->value.audio.base.codec_type) {
            case AUDIO_CODEC_MP3:
                __s_audio_codec_type = AI_AUDIO_CODEC_MP3;
                break;
            case AUDIO_CODEC_OPUS:
                __s_audio_codec_type = AI_AUDIO_CODEC_OPUS;
                break;
            default:
                __s_audio_codec_type = AI_AUDIO_CODEC_MAX;
                break;
        }
    }
    return OPRT_OK;
}

/** recv media data */
OPERATE_RET __wukong_ai_agent_media_data_cb(AI_PACKET_PT type, CHAR_T *data, UINT_T len, UINT_T total_len)
{
    // TAL_PR_NOTICE("wukong ai agent -> recv media type %d", type);
    OPERATE_RET rt = OPRT_OK;
    if(type == AI_PT_AUDIO) {
        rt = wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TTS_DATA, __s_audio_codec_type, data, len);
    } else if(type == AI_PT_VIDEO) {
        // TBD
    } else if(type == AI_PT_IMAGE) {
        // TBD
    } else if(type == AI_PT_FILE) {
        // TBD
    }
    return rt;
}

/** recv text stream */
OPERATE_RET __wukong_ai_agent_text_cb(AI_TEXT_TYPE_E type, ty_cJSON *root, BOOL_T eof)
{    
    TUYA_CHECK_NULL_RETURN(root, OPRT_INVALID_PARM);

    // TAL_PR_NOTICE("wukong ai agent -> recv text type %d", type);
    // TAL_PR_NOTICE("text content: %s", ty_cJSON_PrintUnformatted(root));
    wukong_ai_text_process(type, root, eof);

    return OPRT_OK;
}

/** recv alert */
OPERATE_RET __wukong_ai_agent_alert_cb(AI_ALERT_TYPE_E type)
{
    // PR_DEBUG("wukong ai agent -> alert type: %d", type); 
    if (type == AT_PLEASE_AGAIN) {
        PR_DEBUG("ignored alert: %d", type);
        return OPRT_OK;
    }

    wukong_ai_event_notify(WUKONG_AI_EVENT_PLAY_ALERT, (VOID*)type);   
    return OPRT_OK;
}

OPERATE_RET wukong_ai_agent_init(WUKONG_EVENT_OUTPUT cb)
{
    OPERATE_RET rt = OPRT_OK;
    AI_AGENT_CFG_T ai_agent_cfg = {0};
#if defined(ENABLE_APP_OPUS_ENCODER) && (ENABLE_APP_OPUS_ENCODER == 1)
    ai_agent_cfg.attr.audio.codec_type = AUDIO_CODEC_OPUS;
#elif defined(ENABLE_APP_SPEEX_ENCODER) && (ENABLE_APP_SPEEX_ENCODER == 1)
    ai_agent_cfg.attr.audio.codec_type = AUDIO_CODEC_SPEEX;
#else
    ai_agent_cfg.attr.audio.codec_type = AUDIO_CODEC_PCM;
#endif
    ai_agent_cfg.attr.audio.sample_rate = 16000;
    ai_agent_cfg.attr.audio.channels = AUDIO_CHANNELS_MONO;
    ai_agent_cfg.attr.audio.bit_depth = 16;

#ifdef ENABLE_TUYA_CAMERA
    ai_agent_cfg.attr.video.codec_type = VIDEO_CODEC_H264;
    ai_agent_cfg.attr.video.sample_rate = 90000;
    ai_agent_cfg.attr.video.fps = 30;
    ai_agent_cfg.attr.video.width = 480;
    ai_agent_cfg.attr.video.height = 480;
#endif

    ai_agent_cfg.output.alert_cb = __wukong_ai_agent_alert_cb;
    ai_agent_cfg.output.text_cb = __wukong_ai_agent_text_cb;
    ai_agent_cfg.output.media_data_cb = __wukong_ai_agent_media_data_cb;
    ai_agent_cfg.output.media_attr_cb = __wukong_ai_agent_media_attr_cb;
    ai_agent_cfg.output.event_cb = __wukong_ai_agent_event_cb;
#if defined(USING_UART_AUDIO_INPUT) && USING_UART_AUDIO_INPUT==1
    ai_agent_cfg.codec_enable = FALSE;
#if defined(UART_CODEC_UPLOAD_FORMAT) && (UART_CODEC_UPLOAD_FORMAT == 1)
    ai_agent_cfg.attr.audio.codec_type = AUDIO_CODEC_SPEEX;
#else
    ai_agent_cfg.attr.audio.codec_type = AUDIO_CODEC_OPUS;
#endif
#else
    ai_agent_cfg.codec_enable = TRUE;
#endif

#ifdef AI_PLAYER_DECODER_OPUS_ENABLE
    ai_agent_cfg.tts_cfg.format = "opus";
    ai_agent_cfg.tts_cfg.sample_rate = 16000;
    ai_agent_cfg.tts_cfg.bit_rate = AI_PLAYER_DECODER_OPUS_KBPS * 1000;
#endif
    TUYA_CALL_ERR_RETURN(tuya_ai_agent_init(&ai_agent_cfg));
    __s_event_notify_cb = cb;
    
    return rt;
}

OPERATE_RET wukong_ai_agent_deinit(VOID)
{
    __s_event_notify_cb = NULL;
    tuya_ai_agent_deinit();
    return OPRT_OK;
}

OPERATE_RET wukong_ai_agent_send_text(CHAR_T *content)
{
    OPERATE_RET rt = OPRT_OK;
    // tuya_ai_input_start(TRUE);
    TUYA_CALL_ERR_RETURN(tuya_ai_text_input((BYTE_T *)content, strlen(content), strlen(content)));
    // tuya_ai_input_stop();
    return rt;
}

OPERATE_RET wukong_ai_agent_send_file(UINT8_T *data, UINT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    // tuya_ai_input_start(TRUE);
    TUYA_CALL_ERR_RETURN(tuya_ai_file_input((BYTE_T *)data, len, len));
    // tuya_ai_input_stop();
    return rt;
}

OPERATE_RET wukong_ai_agent_send_image(UINT8_T *data, UINT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    UINT64_T   timestamp = tal_system_get_millisecond();
    // tuya_ai_input_start(TRUE);
    TUYA_CALL_ERR_RETURN(tuya_ai_image_input(timestamp, (BYTE_T *)data, len, len));
    // tuya_ai_input_stop();
    return rt;
}


OPERATE_RET wukong_ai_agent_send_video(UINT8_T *data, UINT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    UINT64_T   pts = 0;
    UINT64_T   timestamp = 0;
    pts = timestamp = tal_system_get_millisecond();
    // tuya_ai_input_start(TRUE);
    TUYA_CALL_ERR_RETURN(tuya_ai_video_input(timestamp, pts, data, len, len));
    // tuya_ai_input_stop();
    return rt;
}

OPERATE_RET wukong_ai_agent_cloud_alert(INT_T type)
{
    OPERATE_RET rt = OPRT_OK;
    TAL_PR_NOTICE("wukong ai agent -> request cloud request for %d", type);
    CHAR_T *alert_prompt = NULL;
    switch (type) {
    case AT_NETWORK_CONNECTED:
        alert_prompt = "cmd:0";
        break;
    // case AT_PLEASE_AGAIN:
    //     alert_prompt = "提示音：联网";
    //     // ty_cJSON_AddStringToObject(json, "eventType", "pleaseAgain");
    //     break;
    case AT_WAKEUP:
        alert_prompt = "cmd:1";
        break;
    case AT_LONG_KEY_TALK:
        alert_prompt = "cmd:2";
        break;
    case AT_KEY_TALK:
        alert_prompt = "cmd:3";
        break;
    case AT_WAKEUP_TALK:
        alert_prompt = "cmd:4";
        break;
    case AT_RANDOM_TALK:
        alert_prompt = "cmd:5";
        break;
    default:
        return OPRT_NOT_SUPPORTED;
    }
    
    TUYA_CALL_ERR_LOG(wukong_ai_agent_send_text(alert_prompt));
    return rt;
}


OPERATE_RET wukong_ai_agent_role_switch(CHAR_T *role)
{
    OPERATE_RET rt = OPRT_OK;
    ty_cJSON* result = NULL;
    // CHAR_T *print_data = NULL;
    CHAR_T post_content[128] = {0};
    sprintf(post_content, "{\"commandInfo\": \"%s\"}", role);
	
  	// 发起切换
    TUYA_CALL_ERR_LOG(iot_httpc_common_post_simple("thing.ai.agent.switch.role", "1.0", post_content, NULL, &result));
    TUYA_CHECK_NULL_RETURN(result, OPRT_MID_HTTP_GET_RESP_ERROR);

  	// 释放资源
    ty_cJSON_Delete(result);
    return OPRT_OK;    
}

VOID wukong_ai_event_notify(WUKONG_AI_EVENT_TYPE_E type, VOID *data)
{
    WUKONG_AI_EVENT_T event = {0};
    if (__s_event_notify_cb) {
        event.data = data;
        event.type = type;
        __s_event_notify_cb(&event);
    }

    return;
}
