#include "wukong_ai_skills.h"
#include "wukong_audio_player.h"
#include "wukong_picture_output.h"
#include "skill_emotion.h"
#include "skill_music_story.h"
#include "skill_cloudevent.h"
#include "skill_clock.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "ty_cJSON.h"
#include <stdio.h>

STATIC BOOL_T __s_chat_break = FALSE;

STATIC CONST CHAR_T *__wukong_ai_map_emotion_for_ui(CONST CHAR_T *name)
{
    CHAR_T normalized[32] = {0};
    UINT_T i = 0;

    if (NULL == name || '\0' == name[0]) {
        return "neutral";
    }

    for (i = 0; i < sizeof(normalized) - 1 && '\0' != name[i]; i++) {
        CHAR_T c = name[i];
        if (c >= 'A' && c <= 'Z') {
            normalized[i] = c - 'A' + 'a';
        } else {
            normalized[i] = c;
        }
    }

    if (0 == strcmp(normalized, "surprising") || 0 == strcmp(normalized, "surprised")) {
        strncpy(normalized, "suprising", sizeof(normalized) - 1);
    }

    // The eyes board only drives a single-eye screen here, so normalize cloud
    // emotions into the 128x128 single-eye asset subset.
    if (0 == strcmp(normalized, "neutral")) {
        return "neutral";
    }

    if (0 == strcmp(normalized, "loving") ||
        0 == strcmp(normalized, "kissy") ||
        0 == strcmp(normalized, "touch")) {
        return "loving";
    }

    if (0 == strcmp(normalized, "happy") ||
        0 == strcmp(normalized, "funny") ||
        0 == strcmp(normalized, "laughing") ||
        0 == strcmp(normalized, "winking")) {
        // User wants wink to reuse the happy single-eye animation.
        return "happy";
    }

    if (0 == strcmp(normalized, "thinking")) {
        return "thinking";
    }

    if (0 == strcmp(normalized, "confused") ||
        0 == strcmp(normalized, "cool") ||
        0 == strcmp(normalized, "confident") ||
        0 == strcmp(normalized, "embarrassed") ||
        0 == strcmp(normalized, "silly")) {
        return "confused";
    }

    if (0 == strcmp(normalized, "delicious")) {
        return "delicious";
    }

    if (0 == strcmp(normalized, "relaxed") || 0 == strcmp(normalized, "sleepy")) {
        return "neutral";
    }

    if (0 == strcmp(normalized, "disappointed") ||
        0 == strcmp(normalized, "sad") ||
        0 == strcmp(normalized, "crying")) {
        return "sad";
    }

    if (0 == strcmp(normalized, "surprise") ||
        0 == strcmp(normalized, "suprising") ||
        0 == strcmp(normalized, "shocked")) {
        return "surprise";
    }

    if (0 == strcmp(normalized, "fearful")) {
        return "fearful";
    }

    if (0 == strcmp(normalized, "angry")) {
        return "angry";
    }

    if (0 == strcmp(normalized, "neutral")) {
        return "neutral";
    }

    return "neutral";
}

STATIC VOID __wukong_ai_emit_emotion_by_name(CONST CHAR_T *name)
{
    CONST CHAR_T *mapped_name = __wukong_ai_map_emotion_for_ui(name);
    WUKONG_AI_EMO_T emo = {0};

    emo.name = (CHAR_T *)mapped_name;
    emo.emoji = (CHAR_T *)wukong_emoji_get_by_name(mapped_name);
    wukong_ai_event_notify(WUKONG_AI_EVENT_EMOTION, &emo);
}

STATIC VOID __wukong_ai_emit_emotion_by_emoji(CONST CHAR_T *emoji)
{
    CONST CHAR_T *mapped_name = NULL;
    WUKONG_AI_EMO_T emo = {0};

    if (NULL == emoji || '\0' == emoji[0]) {
        return;
    }

    mapped_name = __wukong_ai_map_emotion_for_ui(wukong_emoji_get_name(emoji));
    emo.emoji = (CHAR_T *)wukong_emoji_get_by_name(mapped_name);
    emo.name = (CHAR_T *)mapped_name;
    wukong_ai_event_notify(WUKONG_AI_EVENT_EMOTION, &emo);
}
OPERATE_RET __wukong_ai_skill_process(AI_TEXT_TYPE_E type, ty_cJSON *root, BOOL_T eof)
{
    OPERATE_RET rt = OPRT_OK;
    CONST ty_cJSON *node = NULL;
    CONST CHAR_T *code = NULL;

    //! root is data:{}, parse code
    node = ty_cJSON_GetObjectItem(root, "code");
    code = ty_cJSON_GetStringValue(node);
    if (!code) 
        return OPRT_OK;
    // ty_cJSON_PrintUnformatted(root);
    TAL_PR_NOTICE("wukong text -> skill code: %s", ty_cJSON_PrintUnformatted(root));
    if (strcmp(code, "music") == 0 ||
               strcmp(code, "story") == 0) {
        WUKONG_AI_MUSIC_T *music = NULL;
        if (wukong_ai_parse_music(root, &music) == OPRT_OK) {
            wukong_ai_parse_music_dump(music);
            wukong_ai_play_music(music);
            wukong_ai_parse_music_free(music);
        }
    } else if (strcmp(code, "PlayControl") == 0) {
        WUKONG_AI_MUSIC_T *music = NULL;
        if ((rt = wukong_ai_parse_playcontrol(root, &music)) == 0) {
            wukong_ai_parse_music_dump(music);
            wukong_ai_play_music(music);
            wukong_ai_parse_music_free(music);
        }
    } else if (strcmp(code, "emo") == 0) {
        ty_cJSON *skill_content = ty_cJSON_GetObjectItem(root, "skillContent");
        ty_cJSON *emotion_array = skill_content ? ty_cJSON_GetObjectItem(skill_content, "emotion") : NULL;
        CHAR_T *emotion_name = NULL;

        if (emotion_array && ty_cJSON_IsArray(emotion_array) && ty_cJSON_GetArraySize(emotion_array) > 0) {
            emotion_name = ty_cJSON_GetStringValue(ty_cJSON_GetArrayItem(emotion_array, 0));
        }

        if (emotion_name && strlen(emotion_name)) {
            __wukong_ai_emit_emotion_by_name(emotion_name);
        } else {
            TAL_PR_NOTICE("skill emo has no valid emotion");
        }
    } else {
        TAL_PR_NOTICE("skill %s not handled", code);
        // TAL_PR_NOTICE("skill content %s ", ty_cJSON_PrintUnformatted(root));

        wukong_ai_event_notify(WUKONG_AI_EVENT_SKILL, root);
    }

    return OPRT_OK; 
}

OPERATE_RET __wukong_ai_asr_process(AI_TEXT_TYPE_E type, ty_cJSON *root, BOOL_T eof)
{
    // ty_cJSON *data = ty_cJSON_GetObjectItem(root, "data");
    // TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);
    CHAR_T *content =  ty_cJSON_GetStringValue(root);
    TAL_PR_NOTICE("wukong text -> ASR result: %s", content);
    
    // send data to register cb
    WUKONG_AI_TEXT_T text;
    text.data      = content;
    text.datalen   = strlen(content);
    text.timeindex = 0;
    wukong_ai_event_notify((0 == strlen(content))?WUKONG_AI_EVENT_ASR_EMPTY:WUKONG_AI_EVENT_ASR_OK, &text);
    return OPRT_OK;
}


OPERATE_RET __wukong_ai_images_process(cJSON *images)
{
    cJSON *url_array = cJSON_GetObjectItem(images, "url");
    if (NULL == url_array || !cJSON_IsArray(url_array)) {
        TAL_PR_ERR("no url array found");
        return OPRT_COM_ERROR;
    }

    INT_T i = 0;
    INT_T url_count = cJSON_GetArraySize(url_array);
    for (i = 0; i < url_count; i++) {
        cJSON *url_item = cJSON_GetArrayItem(url_array, i);
        if (NULL == url_item) {
            TAL_PR_ERR("url item is null");
            continue;
        }

        CHAR_T *url_str = cJSON_GetStringValue(url_item);
        if (NULL == url_str) {
            TAL_PR_ERR("url string is null");
            continue;
        }

        TAL_PR_NOTICE("image url[%d]: %s", i, url_str);
        wukong_picture_output_process(url_str);
    }

    return OPRT_OK;
}

//{"bizId":"micro_chat_vdevo176101510735192_1764153315615","bizType":"NLG","eof":0,
//"data":{"content":"😆","reasoningContent":"","appendMode":"append","timeIndex":400,"finish":false,"tags":"U+1F606"}
OPERATE_RET __wukong_ai_nlg_process(AI_TEXT_TYPE_E type, ty_cJSON *root, BOOL_T eof)
{
    CHAR_T *json_str = ty_cJSON_PrintUnformatted(root);
    TAL_PR_NOTICE("json-str %s", json_str);
    tal_free(json_str);

    // ty_cJSON *time = ty_cJSON_GetObjectItem(root, "timeIndex");
    CHAR_T *content = ty_cJSON_GetStringValue(ty_cJSON_GetObjectItem(root, "content"));
    if (!content) {
        content = "";
    }

    WUKONG_AI_TEXT_T text;
    text.data      = content;
    text.datalen   = strlen(content);
    // text.timeindex = time ? time->valueint : 0;
    TAL_PR_NOTICE("wukong text -> NLG eof: %d, content: %s, time: %d", eof, content, text.timeindex);

    // send data to register cb
    STATIC WUKONG_AI_EVENT_TYPE_E event_type = WUKONG_AI_EVENT_TEXT_STREAM_STOP;
    if (__s_chat_break) {   // restart after chat break
        wukong_ai_event_notify(WUKONG_AI_EVENT_TEXT_STREAM_START, &text);
        event_type = WUKONG_AI_EVENT_TEXT_STREAM_DATA;
        __s_chat_break = FALSE;
    } else if (event_type == WUKONG_AI_EVENT_TEXT_STREAM_STOP) {
        wukong_ai_event_notify(WUKONG_AI_EVENT_TEXT_STREAM_START, &text);
        event_type = WUKONG_AI_EVENT_TEXT_STREAM_DATA;
    } else {
        if (event_type == WUKONG_AI_EVENT_TEXT_STREAM_DATA) {
            wukong_ai_event_notify(eof?WUKONG_AI_EVENT_TEXT_STREAM_STOP:WUKONG_AI_EVENT_TEXT_STREAM_DATA, &text);
            event_type = eof?WUKONG_AI_EVENT_TEXT_STREAM_STOP:WUKONG_AI_EVENT_TEXT_STREAM_DATA;
        }
    }

    // emtion
    ty_cJSON *tags = ty_cJSON_GetObjectItem(root, "tags");
    CHAR_T *emotion = NULL;
    if (tags && ty_cJSON_IsArray(tags) && ty_cJSON_GetArraySize(tags) > 0) {
        emotion = ty_cJSON_GetStringValue(ty_cJSON_GetArrayItem(tags, 0));
    } else if (tags && ty_cJSON_IsString(tags)) {
        emotion = ty_cJSON_GetStringValue(tags);
    }

    if (emotion && strlen(emotion)) {
        if ((emotion[0] == 'U' || emotion[0] == 'u') && emotion[1] == '+') {
            __wukong_ai_emit_emotion_by_emoji(emotion);
        } else {
            __wukong_ai_emit_emotion_by_name(emotion);
        }
    }

    // picture
    // cJSON *images = cJSON_GetObjectItem(root, "images");
    // if(images) {
    //     __wukong_ai_images_process(root);
    // }

    return OPRT_OK;
}

VOID  wukong_ai_text_process(AI_TEXT_TYPE_E type, ty_cJSON *root, BOOL_T eof)
{
    switch (type)
    {
    case AI_TEXT_SKILL:
        __wukong_ai_skill_process(type, root, eof);
        break;
    case AI_TEXT_ASR:
        __wukong_ai_asr_process(type, root, eof);
        break;
    case AI_TEXT_NLG:
        __wukong_ai_nlg_process(type, root, eof);
        break;
    case AI_TEXT_CLOUD_EVENT:
        wukong_ai_parse_cloud_event(root);
        break;
    default:
        break;
    }
}

VOID wukong_skill_notify_chat_break()
{
    __s_chat_break = TRUE;
}
