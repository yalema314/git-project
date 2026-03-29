#include "wukong_ai_mode_hold.h"

#if defined(ENABLE_AI_MODE_HOLD) && (ENABLE_AI_MODE_HOLD == 1)
STATIC AI_CHAT_MODE_HANDLE_T s_ai_hold_cb = {0};
STATIC AI_HOLD_PARAM_T s_ai_hold = {0};
STATIC AI_CHAT_STATE_E s_ai_cur_state = AI_CHAT_INVALID;

STATIC VOID __ai_hold_asr_result(WUKONG_AI_EVENT_TYPE_E type, UCHAR_T *data)
{
    if (NULL == data) {
        return;
    }

    WUKONG_AI_TEXT_T *text = (WUKONG_AI_TEXT_T *)data;
    //TAL_PR_NOTICE("ai toy -> recv wukong asr result: %s", text->data);
#ifdef ENABLE_TUYA_UI
    tuya_ai_display_msg((UINT8_T*)text->data, text->datalen, TY_DISPLAY_TP_HUMAN_CHAT);
#endif
    return;    
}

STATIC VOID __ai_hold_text_stream(WUKONG_AI_EVENT_TYPE_E type, UCHAR_T *data)
{
    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);

    WUKONG_AI_TEXT_T *text = (WUKONG_AI_TEXT_T *)data;
    TAL_PR_NOTICE("ai toy -> recv wukong text result: %s", text->data);

    switch (type)
    {
    case WUKONG_AI_EVENT_TEXT_STREAM_START:
        #ifdef ENABLE_TUYA_UI
        tuya_ai_display_msg(text->data, text->datalen,  TY_DISPLAY_TP_AI_CHAT_START);
        #endif
        break;
    case WUKONG_AI_EVENT_TEXT_STREAM_DATA:
        #ifdef ENABLE_TUYA_UI
        tuya_ai_display_msg(text->data, text->datalen, TY_DISPLAY_TP_AI_CHAT_DATA);
        #endif
        break;
    default:
        break;
    }
}

STATIC VOID __ai_hold_emition(WUKONG_AI_EVENT_TYPE_E type, UCHAR_T *data)
{
    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);

    WUKONG_AI_EMO_T *emo = (WUKONG_AI_EMO_T *)data;
    TAL_PR_NOTICE("ai toy -> recv wukong emotion result: %s", emo->name);
#ifdef ENABLE_TUYA_UI
    tuya_ai_display_msg((UINT8_T*)emo->name, strlen(emo->name), TY_DISPLAY_TP_EMOJI);
#endif 
    return;
}

STATIC OPERATE_RET __ai_hold_idle_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] idle");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_off();

    //close idle timer
    tuya_ai_toy_idle_timer_ctrl(FALSE);
    //open low power timer
    tuya_ai_toy_lowpower_timer_ctrl(TRUE);

    //disable wakeup
    wukong_audio_input_wakeup_set(FALSE);
    s_ai_hold.wakeup_stat = FALSE;
    return rt;
}

STATIC OPERATE_RET __ai_hold_listen_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] listen");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_flash(500);

    //open idle timer
    tuya_ai_toy_idle_timer_ctrl(TRUE);
    //close low power timer
    tuya_ai_toy_lowpower_timer_ctrl(FALSE);

    //wakeup audio input
    s_ai_hold.wakeup_stat = TRUE;
    wukong_audio_input_wakeup_set(TRUE);

    return rt;
}

STATIC OPERATE_RET __ai_hold_upload_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] upload");
    OPERATE_RET rt = OPRT_OK;
    s_ai_hold.wakeup_stat = FALSE;
    wukong_audio_input_wakeup_set(FALSE);

    // stop vad, stop session
    tuya_ai_input_stop();

    return rt;
}

STATIC OPERATE_RET __ai_hold_think_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] think");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_flash(2000);

    tuya_ai_toy_idle_timer_ctrl(TRUE);
    wukong_audio_input_wakeup_set(FALSE);

    return rt;
}

STATIC OPERATE_RET __ai_hold_speak_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] speak");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_on();
    
    tuya_ai_toy_idle_timer_ctrl(FALSE);
    return rt;
}

STATIC OPERATE_RET wukong_ai_hold_int_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] init");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_on();

    //set vad mode
    wukong_audio_input_wakeup_mode_set(WUKONG_AUDIO_VAD_MANIAL);

    //disenable kws
    wukong_kws_disable();

    // s_ai_hold.state = AI_CHAT_IDLE;
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_IDLE);

    // disable vad
    tuya_ai_agent_server_vad_ctrl(FALSE);

    s_ai_hold.wakeup_stat = FALSE;

#ifdef ENABLE_TUYA_UI
    AI_TRIGGER_MODE_E trigger_mode = AI_TRIGGER_MODE_HOLD;
    tuya_ai_display_msg(&trigger_mode, 1, TY_DISPLAY_TP_CHAT_MODE);
#endif

    return rt;
}

STATIC OPERATE_RET wukong_ai_hold_deint_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] deinit");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_input_stop();
    s_ai_cur_state = AI_CHAT_INVALID;
    // enable vad
    tuya_ai_agent_server_vad_ctrl(TRUE);
    return rt;
}

STATIC OPERATE_RET wukong_ai_hold_task_cb(VOID *data, INT_T len)
{
    if(s_ai_cur_state == s_ai_hold.state)
    {
        return OPRT_OK;
    }

    switch(s_ai_hold.state)
    {
        case AI_CHAT_INIT:
        {
            ;
        }
        break;

        case AI_CHAT_IDLE:
        {
            __ai_hold_idle_cb(NULL, 0);
        }
        break;

        case AI_CHAT_LISTEN:
        {
            __ai_hold_listen_cb(NULL, 0);
        }
        break;

        case AI_CHAT_UPLOAD:
        {
            __ai_hold_upload_cb(NULL, 0);
        }
        break;

        case AI_CHAT_THINK:
        {
            __ai_hold_think_cb(NULL, 0);
        }
        break;

        case AI_CHAT_SPEAK:
        {
            __ai_hold_speak_cb(NULL, 0);
        }
        break;

        default:
        break;
    }

    s_ai_cur_state = s_ai_hold.state;

#ifdef ENABLE_TUYA_UI   
    tuya_ai_display_msg(&s_ai_hold.state, 1, TY_DISPLAY_TP_CHAT_STAT);
#endif  

    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_hold_event_cb(VOID *data, INT_T len)
{
    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);
    WUKONG_AI_EVENT_T *event = (WUKONG_AI_EVENT_T *)data;

    // STATIC WUKONG_AI_EVENT_TYPE_E cur = WUKONG_AI_EVENT_IDLE;
    // if(cur == event->type)
    // {
    //     return OPRT_OK;
    // }
    // cur = event->type;

    TAL_PR_DEBUG("[====ai_hold] event type: %d", event->type);
    switch (event->type) 
    {
        case WUKONG_AI_EVENT_ASR_EMPTY:
        case WUKONG_AI_EVENT_ASR_ERROR:
        {
            // s_ai_hold.state = AI_CHAT_IDLE;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_IDLE);
        }
        break;

        case WUKONG_AI_EVENT_ASR_OK:
        {
            // s_ai_hold.state = AI_CHAT_THINK;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_THINK);
            __ai_hold_asr_result(event->type, event->data);
        }
        break;
            
        case WUKONG_AI_EVENT_TTS_PRE:
        {
            // s_ai_hold.state = AI_CHAT_SPEAK;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_SPEAK);
        }
        break;

        case WUKONG_AI_EVENT_TTS_START:
        case WUKONG_AI_EVENT_TTS_DATA:   
        {

        }
        break;   

        case WUKONG_AI_EVENT_TTS_STOP:
        case WUKONG_AI_EVENT_TTS_ABORT:
        case WUKONG_AI_EVENT_TTS_ERROR:
        {

        }
        break;

        case WUKONG_AI_EVENT_VAD_TIMEOUT:
        {

        }
        break;

        case WUKONG_AI_EVENT_TEXT_STREAM_START:
        case WUKONG_AI_EVENT_TEXT_STREAM_DATA:
        case WUKONG_AI_EVENT_TEXT_STREAM_STOP:
        case WUKONG_AI_EVENT_TEXT_STREAM_ABORT:
        {
            __ai_hold_text_stream(event->type, event->data);
        }
        break;

        case WUKONG_AI_EVENT_EMOTION:
        case WUKONG_AI_EVENT_LLM_EMOTION:
        {
            __ai_hold_emition(event->type, event->data);
        }
        break;

        case WUKONG_AI_EVENT_SKILL:
        {

        }
        break;

        case WUKONG_AI_EVENT_CHAT_BREAK:
        case WUKONG_AI_EVENT_SERVER_VAD:
        {

        }
        break;

        case WUKONG_AI_EVENT_PLAY_CTL_PLAY:
        case WUKONG_AI_EVENT_PLAY_CTL_RESUME:
        {
            wukong_audio_player_set_resume(TRUE);
        }
        break;

        case WUKONG_AI_EVENT_PLAY_CTL_PAUSE:
        {
            // wukong_audio_player_set_resume(FALSE);
            // wukong_audio_player_set_replay(FALSE);
            wukong_audio_player_stop(AI_PLAYER_BG);
        }
        break;

        case WUKONG_AI_EVENT_PLAY_CTL_REPLAY:
        {
            wukong_audio_player_set_replay(TRUE);
        }
        break;

        case WUKONG_AI_EVENT_PLAY_CTL_END:
        case WUKONG_AI_EVENT_PLAY_END:
        {
            if(s_ai_hold.wakeup_stat)
            {
                // s_ai_hold.state = AI_CHAT_LISTEN;
                MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_LISTEN);
            }
            else
            {
                // s_ai_hold.state = AI_CHAT_IDLE;
                MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_IDLE);
            }
        }
        break;        

        case WUKONG_AI_EVENT_PLAY_ALERT:
        {
            wukong_audio_player_alert((TY_AI_TOY_ALERT_TYPE_E)event->data, TRUE);
        }
        break;

        case WUKONG_AI_EVENT_PLAY_CTL_PREV:
        case WUKONG_AI_EVENT_PLAY_CTL_NEXT:
        case WUKONG_AI_EVENT_PLAY_CTL_SEQUENTIAL:
        case WUKONG_AI_EVENT_PLAY_CTL_SEQUENTIAL_LOOP:
        case WUKONG_AI_EVENT_PLAY_CTL_SINGLE_LOOP:
        {

        }
        break;

        default:
        break;
    }

    return;
}

STATIC OPERATE_RET wukong_ai_hold_wakeup(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] wakeup"); 
    OPERATE_RET rt = OPRT_OK;
    return rt;
}

STATIC OPERATE_RET wukong_ai_hold_vad(VOID *data, INT_T len)
{
    // if(!s_ai_hold.wakeup_stat)
    // {
    //     return OPRT_OK;
    // }

    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);
    OPERATE_RET rt = OPRT_OK;

    WUKONG_AUDIO_VAD_FLAG_E vad_flag = (WUKONG_AUDIO_VAD_FLAG_E)data;
    TAL_PR_DEBUG("[====ai_hold] vad: [%d]", vad_flag); 
    if (WUKONG_AUDIO_VAD_START == vad_flag) 
    {
        if (!tuya_ai_agent_is_ready()) {
            TAL_PR_DEBUG("ai agent is not ready, ignore audio input");
            return OPRT_RESOURCE_NOT_READY;
        }
        tuya_ai_agent_set_scode(AI_AGENT_SCODE_DEFAULT);
        tuya_ai_input_start(FALSE);
    } 
    else 
    {
        tuya_ai_input_stop();
        // s_ai_hold.state = AI_CHAT_UPLOAD;
    }

    return rt;
}

STATIC OPERATE_RET wukong_ai_hold_client_run(VOID_T *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] client run");
    // s_ai_hold.state = AI_CHAT_IDLE;
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_IDLE);
    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_hold_key_cb(VOID *data, INT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    PUSH_KEY_TYPE_E event = *(PUSH_KEY_TYPE_E *)data;
    TAL_PR_DEBUG("[====ai_hold] key: %d", event);
    switch (event) 
    {        
        case NORMAL_KEY:
        {
            wukong_audio_player_stop(AI_PLAYER_ALL);
            wukong_audio_input_reset();
            tuya_ai_agent_event(AI_EVENT_CHAT_BREAK, 0);
            // s_ai_hold.state = AI_CHAT_IDLE;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_IDLE);
            s_ai_hold.wakeup_stat = FALSE;

        } 
        break;  

        case SEQ_KEY:
        {
            ;
        }
        break;

        case LONG_KEY: 
        {
            // s_ai_hold.state = AI_CHAT_LISTEN;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_LISTEN);
            // s_ai_hold.wakeup_stat = TRUE;
        }
        break;   

        case RELEASE_KEY: 
        {
            // s_ai_hold.state = AI_CHAT_UPLOAD;    
            if (s_ai_hold.wakeup_stat) {
                MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_UPLOAD);
            }
        }
        break;

        default:
        break;
    }
    
    return rt;
}

STATIC OPERATE_RET wukong_ai_hold_notify_idle_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_hold] client run");
    OPERATE_RET rt = OPRT_OK;

    // s_ai_hold.state = AI_CHAT_IDLE;
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_HOLD, s_ai_hold.state, AI_CHAT_IDLE);

    return rt;
}

OPERATE_RET ai_hold_register(AI_CHAT_MODE_HANDLE_T **cb)
{
    OPERATE_RET rt = OPRT_OK;

    s_ai_hold_cb.ai_handle_init      = wukong_ai_hold_int_cb;
    s_ai_hold_cb.ai_handle_deinit    = wukong_ai_hold_deint_cb;
    s_ai_hold_cb.ai_handle_key       = wukong_ai_hold_key_cb;
    s_ai_hold_cb.ai_handle_task      = wukong_ai_hold_task_cb;
    s_ai_hold_cb.ai_handle_event     = wukong_ai_hold_event_cb;
    s_ai_hold_cb.ai_handle_wakeup    = wukong_ai_hold_wakeup;
    s_ai_hold_cb.ai_handle_vad       = wukong_ai_hold_vad;
    s_ai_hold_cb.ai_handle_client    = wukong_ai_hold_client_run;

    s_ai_hold_cb.ai_notify_idle      = wukong_ai_hold_notify_idle_cb;
    *cb = &s_ai_hold_cb;
    return rt;
}
#endif
