#include "wukong_ai_mode.h"
#include "wukong_ai_mode_free.h"
#include "wukong_ai_mode_hold.h"
#include "wukong_ai_mode_oneshot.h"
#include "wukong_ai_mode_wakeup.h"
#include "tuya_ai_input.h"

STATIC AI_MODE_SWITCH_T s_ai_mode_map[AI_TRIGGER_MODE_MAX] = {0};
STATIC AI_MODE_SWITCH_T s_ai_chat = {0};

CHAR_T *_mode_str[] = {
    "hold", "oneshot", "wakeup", "free", "p2p", "trans", "Picture", "unknown",
};

CHAR_T *_state_str[] = {
    "INIT", "IDLE", "LISTEN", "UPLOAD", "THINK", "SPEAK", "UNKNOWN"
};

// switch between "hold", "oneshot", "wakeup", "free"
OPERATE_RET wukong_ai_mode_switch(AI_TRIGGER_MODE_E cur_mode)
{
    AI_TRIGGER_MODE_E new_mode = (cur_mode + 1) % AI_TRIGGER_MODE_MAX; 
    INT_T count = 0;

    do
    {
        if(s_ai_mode_map[new_mode].enabled)
        {
            TAL_PR_DEBUG("[====ai_mode] change ai mode %d to %d\n", cur_mode, new_mode);
            wukong_ai_handle_deinit(NULL, 0);
            memcpy(&s_ai_chat, &s_ai_mode_map[new_mode], sizeof(AI_MODE_SWITCH_T));
            tuya_ai_toy_trigger_mode_set(new_mode);
            wukong_ai_handle_init(NULL, 0);
            return new_mode;
        }
        new_mode = (new_mode + 1) % AI_TRIGGER_MODE_MAX;

    }while(count++ < AI_TRIGGER_MODE_MAX);

    TAL_PR_ERR("[====ai_mode] no valid ai mode\n");
    return OPRT_NOT_SUPPORTED;
}

// switch to anly mode
OPERATE_RET wukong_ai_mode_switch_to(AI_TRIGGER_MODE_E mode)
{
    if(s_ai_mode_map[mode].enabled)
    {
        TAL_PR_DEBUG("[====ai_mode] switch ai mode to %d\n", mode);
        wukong_ai_handle_deinit(NULL, 0);
        memcpy(&s_ai_chat, &s_ai_mode_map[mode], sizeof(AI_MODE_SWITCH_T));
		tuya_ai_toy_trigger_mode_set(mode);
        wukong_ai_handle_init(NULL, 0);
        return OPRT_OK;
    }
    else
    {
        TAL_PR_ERR("[====ai_mode] ai mode %d not enabled\n", mode);
        return OPRT_NOT_SUPPORTED;
    }
}
OPERATE_RET wukong_ai_handle_init(VOID *data, INT_T len)
{
    if(s_ai_chat.handle->ai_handle_init)
    {
        return s_ai_chat.handle->ai_handle_init(data, len);
    }
    return OPRT_NOT_FOUND;
}

OPERATE_RET wukong_ai_handle_deinit(VOID *data, INT_T len)
{
    if(s_ai_chat.handle->ai_handle_deinit)
    {
        return s_ai_chat.handle->ai_handle_deinit(data, len);
    }
    return OPRT_NOT_FOUND;  
}

OPERATE_RET wukong_ai_handle_key(VOID *data, INT_T len)
{
    if(s_ai_chat.handle && s_ai_chat.handle->ai_handle_key)
    {
        return s_ai_chat.handle->ai_handle_key(data, len);
    }
    return OPRT_NOT_FOUND;  
}

OPERATE_RET wukong_ai_handle_task(VOID *data, INT_T len)
{
    if(s_ai_chat.handle && s_ai_chat.handle->ai_handle_task)
    {
        return s_ai_chat.handle->ai_handle_task(data, len);
    }
    return OPRT_NOT_FOUND;  
}

OPERATE_RET wukong_ai_handle_event(VOID *data, INT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    if(s_ai_chat.handle && s_ai_chat.handle->ai_handle_event)
    {
        tal_mutex_lock(s_ai_chat.mutex);
        s_ai_chat.handle->ai_handle_event(data, len);
        tal_mutex_unlock(s_ai_chat.mutex);
        return rt;
    }
    return OPRT_NOT_FOUND;  
}

OPERATE_RET wukong_ai_handle_wakeup(VOID *data, INT_T len)
{
    if(s_ai_chat.handle && s_ai_chat.handle->ai_handle_wakeup)
    {
        return s_ai_chat.handle->ai_handle_wakeup(data, len);
    }
    return OPRT_NOT_FOUND;  
}

OPERATE_RET wukong_ai_handle_vad(VOID *data, INT_T len)
{
    if(s_ai_chat.handle && s_ai_chat.handle->ai_handle_vad)
    {
        return s_ai_chat.handle->ai_handle_vad(data, len);
    }
    return OPRT_NOT_FOUND;  
}

OPERATE_RET wukong_ai_handle_client(VOID *data, INT_T len)
{
    if(s_ai_chat.handle && s_ai_chat.handle->ai_handle_client)
    {
        return s_ai_chat.handle->ai_handle_client(data, len);
    }
    return OPRT_NOT_FOUND;  
}

OPERATE_RET wukong_ai_notify_idle(VOID *data, INT_T len)
{
    if(s_ai_chat.handle && s_ai_chat.handle->ai_notify_idle)
    {
        return s_ai_chat.handle->ai_notify_idle(data, len);
    }
    return OPRT_NOT_FOUND;  
}

STATIC OPERATE_RET __default_audio_input(VOID *data, INT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    UINT64_T   pts = 0;
    UINT64_T   timestamp = 0;

    TAL_PR_NOTICE("ai toy -> recv wukong mic data: %d", len);

    if (!tuya_ai_agent_is_ready()) {
        TAL_PR_DEBUG("ai agent is not ready, ignore audio input");
        return OPRT_OK;
    }

    timestamp = pts = tal_system_get_millisecond();
    TUYA_CALL_ERR_LOG(tuya_ai_audio_input(timestamp, pts, (UINT8_T *)data, len, len));

#if 0
    TKL_AUDIO_FRAME_INFO_T frame_info = {
        .pbuf = msg->data, 
        .used_size = msg->datalen,
    };
    TAL_PR_NOTICE("ai toy -> upload data: %d", msg->datalen);
    tkl_ao_put_frame(TKL_AUDIO_TYPE_BOARD, 0, NULL, &frame_info);
#endif
    return OPRT_OK;
}

OPERATE_RET wukong_ai_handle_audio_input(VOID *data, INT_T len)
{
    if(s_ai_chat.handle->ai_handle_audio_input)
    {
        return s_ai_chat.handle->ai_handle_audio_input(data, len);
    }
    else{
        return __default_audio_input(data, len);
    }
    return OPRT_NOT_FOUND;  
}
OPERATE_RET wukong_ai_mode_init()
{
//长按
#if defined(ENABLE_AI_MODE_HOLD) && (ENABLE_AI_MODE_HOLD == 1)
    s_ai_mode_map[AI_TRIGGER_MODE_HOLD].enabled = TRUE;
    tal_mutex_create_init(&s_ai_mode_map[AI_TRIGGER_MODE_HOLD].mutex );
    s_ai_mode_map[AI_TRIGGER_MODE_HOLD].trigger_mode = AI_TRIGGER_MODE_HOLD;
    ai_hold_register(&s_ai_mode_map[AI_TRIGGER_MODE_HOLD].handle);
#else 
    s_ai_mode_map[AI_TRIGGER_MODE_HOLD].enabled = FALSE;
    s_ai_mode_map[AI_TRIGGER_MODE_HOLD].trigger_mode = AI_TRIGGER_MODE_HOLD;
    s_ai_mode_map[AI_TRIGGER_MODE_HOLD].handle = NULL;
#endif

//短按
#if defined(ENABLE_AI_MODE_ONESHOT) && (ENABLE_AI_MODE_ONESHOT == 1)
    s_ai_mode_map[AI_TRIGGER_MODE_ONE_SHOT].enabled = TRUE;
    tal_mutex_create_init(&s_ai_mode_map[AI_TRIGGER_MODE_ONE_SHOT].mutex );
    s_ai_mode_map[AI_TRIGGER_MODE_ONE_SHOT].trigger_mode = AI_TRIGGER_MODE_ONE_SHOT;
    ai_oneshot_register(&s_ai_mode_map[AI_TRIGGER_MODE_ONE_SHOT].handle);
#else 
    s_ai_mode_map[AI_TRIGGER_MODE_ONE_SHOT].enabled = FALSE;
    s_ai_mode_map[AI_TRIGGER_MODE_ONE_SHOT].trigger_mode = AI_TRIGGER_MODE_ONE_SHOT;
    s_ai_mode_map[AI_TRIGGER_MODE_ONE_SHOT].handle = NULL;
#endif

//唤醒
#if defined(ENABLE_AI_MODE_WAKEUP) && (ENABLE_AI_MODE_WAKEUP == 1)
    s_ai_mode_map[AI_TRIGGER_MODE_WAKEUP].enabled = TRUE;
    tal_mutex_create_init(&s_ai_mode_map[AI_TRIGGER_MODE_WAKEUP].mutex );
    s_ai_mode_map[AI_TRIGGER_MODE_WAKEUP].trigger_mode = AI_TRIGGER_MODE_WAKEUP;
    ai_wakeup_register(&s_ai_mode_map[AI_TRIGGER_MODE_WAKEUP].handle);
#else 
    s_ai_mode_map[AI_TRIGGER_MODE_WAKEUP].enabled = FALSE;
    s_ai_mode_map[AI_TRIGGER_MODE_WAKEUP].trigger_mode = AI_TRIGGER_MODE_WAKEUP;
    s_ai_mode_map[AI_TRIGGER_MODE_WAKEUP].handle = NULL;
#endif

//随意
#if defined(ENABLE_AI_MODE_FREE) && (ENABLE_AI_MODE_FREE == 1)
    s_ai_mode_map[AI_TRIGGER_MODE_FREE].enabled = TRUE;
    tal_mutex_create_init(&s_ai_mode_map[AI_TRIGGER_MODE_FREE].mutex );
    s_ai_mode_map[AI_TRIGGER_MODE_FREE].trigger_mode = AI_TRIGGER_MODE_FREE;
    ai_free_register(&s_ai_mode_map[AI_TRIGGER_MODE_FREE].handle);
#else 
    s_ai_mode_map[AI_TRIGGER_MODE_FREE].enabled = FALSE;
    s_ai_mode_map[AI_TRIGGER_MODE_FREE].trigger_mode = AI_TRIGGER_MODE_FREE;
    s_ai_mode_map[AI_TRIGGER_MODE_FREE].handle = NULL;
#endif

//p2p
#if defined(ENABLE_AI_MODE_P2P) && (ENABLE_AI_MODE_P2P == 1)
    s_ai_mode_map[AI_TRIGGER_MODE_P2P].enabled = TRUE;
    s_ai_mode_map[AI_TRIGGER_MODE_P2P].trigger_mode = AI_TRIGGER_MODE_P2P;
    ai_p2p_register(&s_ai_mode_map[AI_TRIGGER_MODE_P2P].handle);
#else 
    s_ai_mode_map[AI_TRIGGER_MODE_P2P].enabled = FALSE;
    s_ai_mode_map[AI_TRIGGER_MODE_P2P].trigger_mode = AI_TRIGGER_MODE_P2P;
    s_ai_mode_map[AI_TRIGGER_MODE_P2P].handle = NULL;
#endif

// translate
#if defined(ENABLE_AI_MODE_TRANSLATE) && (ENABLE_AI_MODE_TRANSLATE == 1)
    s_ai_mode_map[AI_TRIGGER_MODE_TRANSLATE].enabled = TRUE;
    s_ai_mode_map[AI_TRIGGER_MODE_TRANSLATE].trigger_mode = AI_TRIGGER_MODE_TRANSLATE;
    ai_translate_register(&s_ai_mode_map[AI_TRIGGER_MODE_TRANSLATE].handle);
#else 
    s_ai_mode_map[AI_TRIGGER_MODE_TRANSLATE].enabled = FALSE;
    s_ai_mode_map[AI_TRIGGER_MODE_TRANSLATE].trigger_mode = AI_TRIGGER_MODE_TRANSLATE;
    s_ai_mode_map[AI_TRIGGER_MODE_TRANSLATE].handle = NULL;
#endif

    //防止kv异常导致的初始化失败
    AI_TRIGGER_MODE_E ai_mode = s_ai_mode_map[tuya_ai_toy_trigger_mode_get()].enabled ? tuya_ai_toy_trigger_mode_get() : TUYA_AI_CHAT_DEFAULT_MODE;
    INT_T i = 0;
    do{
        if(s_ai_mode_map[ai_mode].enabled){
            TAL_PR_DEBUG("[====ai_mode] init ai mode to %d\n", ai_mode);
            memcpy(&s_ai_chat, &s_ai_mode_map[ai_mode], sizeof(AI_MODE_SWITCH_T));
            tuya_ai_toy_trigger_mode_set(ai_mode);
            wukong_ai_handle_init(NULL, 0);
            return OPRT_OK;
        }
        ai_mode = (ai_mode + 1) % AI_TRIGGER_MODE_MAX;
    } while(i++ < AI_TRIGGER_MODE_MAX);

    return OPRT_NOT_SUPPORTED;
}