#include "wukong_ai_mode_wakeup.h"

#if defined(ENABLE_AI_MODE_WAKEUP) && (ENABLE_AI_MODE_WAKEUP == 1)
STATIC AI_CHAT_MODE_HANDLE_T s_ai_wakeup_cb = {0};
STATIC AI_WAKEUP_PARAM_T s_ai_wakeup = {0};
STATIC AI_CHAT_STATE_E s_ai_cur_state = AI_CHAT_INVALID;

STATIC VOID __ai_wakeup_asr_result(WUKONG_AI_EVENT_TYPE_E type, UCHAR_T *data)
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

STATIC VOID __ai_wakeup_text_stream(WUKONG_AI_EVENT_TYPE_E type, UCHAR_T *data)
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
        /* code */
        break;
    default:
        break;
    }
}

STATIC VOID __ai_wakeup_emition(WUKONG_AI_EVENT_TYPE_E type, UCHAR_T *data)
{
    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);

    WUKONG_AI_EMO_T *emo = (WUKONG_AI_EMO_T *)data;
    TAL_PR_NOTICE("ai toy -> recv wukong emotion result: %s", emo->name);
#ifdef ENABLE_TUYA_UI
    tuya_ai_display_msg((UINT8_T*)emo->name, strlen(emo->name), TY_DISPLAY_TP_EMOJI);
#endif 
    return;
}

STATIC OPERATE_RET __ai_wakeup_idle_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] idle");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_off();

    //close idle timer
    tuya_ai_toy_idle_timer_ctrl(FALSE);
    //open low power timer
    tuya_ai_toy_lowpower_timer_ctrl(TRUE);

    //disable wakeup
    wukong_audio_input_wakeup_set(FALSE);
    s_ai_wakeup.wakeup_stat = FALSE;
    return rt;
}

STATIC OPERATE_RET __ai_wakeup_listen_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] listen");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_flash(500);

    //open idle timer
    tuya_ai_toy_idle_timer_ctrl(TRUE);
    //close low power timer
    tuya_ai_toy_lowpower_timer_ctrl(FALSE);

    //wakeup audio input
    s_ai_wakeup.wakeup_stat = TRUE;
    wukong_audio_input_wakeup_set(TRUE);

    return rt;
}

STATIC OPERATE_RET __ai_wakeup_upload_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] upload");
    OPERATE_RET rt = OPRT_OK;
    return rt;
}

STATIC OPERATE_RET __ai_wakeup_think_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] think");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_flash(2000);

    tuya_ai_toy_idle_timer_ctrl(TRUE);
    s_ai_wakeup.wakeup_stat = FALSE;
    wukong_audio_input_wakeup_set(FALSE);

    return rt;
}

STATIC OPERATE_RET __ai_wakeup_speak_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] speak");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_on();
    
    tuya_ai_toy_idle_timer_ctrl(FALSE);
    return rt;
}

STATIC OPERATE_RET wukong_ai_wakeup_int_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] init");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_on();

    //set vad mode
    wukong_audio_input_wakeup_mode_set(WUKONG_AUDIO_VAD_AUTO);

    //disenable kws
    wukong_kws_enable();

    // s_ai_wakeup.state = AI_CHAT_IDLE;
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_IDLE);

    s_ai_wakeup.wakeup_stat = FALSE;

#ifdef ENABLE_TUYA_UI
    AI_TRIGGER_MODE_E trigger_mode = AI_TRIGGER_MODE_WAKEUP;
    tuya_ai_display_msg(&trigger_mode, 1, TY_DISPLAY_TP_CHAT_MODE);
#endif
    
    return rt;
}

STATIC OPERATE_RET wukong_ai_wakeup_deint_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] deinit");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_input_stop();
    s_ai_cur_state = AI_CHAT_INVALID;
    return rt;
}

STATIC OPERATE_RET wukong_ai_wakeup_task_cb(VOID *data, INT_T len)
{
    if(s_ai_cur_state == s_ai_wakeup.state)
    {
        return OPRT_OK;
    }

    switch(s_ai_wakeup.state)
    {
        case AI_CHAT_INIT:
        {
            ;
        }
        break;

        case AI_CHAT_IDLE:
        {
            __ai_wakeup_idle_cb(NULL, 0);
        }
        break;

        case AI_CHAT_LISTEN:
        {
            __ai_wakeup_listen_cb(NULL, 0);
        }
        break;

        case AI_CHAT_UPLOAD:
        {
            __ai_wakeup_upload_cb(NULL, 0);
        }
        break;

        case AI_CHAT_THINK:
        {
            __ai_wakeup_think_cb(NULL, 0);
        }
        break;

        case AI_CHAT_SPEAK:
        {
            __ai_wakeup_speak_cb(NULL, 0);
        }
        break;

        default:
        break;
    }

    s_ai_cur_state = s_ai_wakeup.state;

#ifdef ENABLE_TUYA_UI   
    tuya_ai_display_msg(&s_ai_wakeup.state, 1, TY_DISPLAY_TP_CHAT_STAT);
#endif  

    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_wakeup_event_cb(VOID *data, INT_T len)
{
    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);
    WUKONG_AI_EVENT_T *event = (WUKONG_AI_EVENT_T *)data;

    // STATIC WUKONG_AI_EVENT_TYPE_E cur = WUKONG_AI_EVENT_IDLE;
    // if(cur == event->type)
    // {
    //     return OPRT_OK;
    // }
    // cur = event->type;

    TAL_PR_DEBUG("[====ai_wakeup] event type: %d", event->type);
    switch (event->type) 
    {
        case WUKONG_AI_EVENT_ASR_EMPTY:
        case WUKONG_AI_EVENT_ASR_ERROR:
        {
            // s_ai_wakeup.state = AI_CHAT_LISTEN;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_LISTEN);
        }
        break;

        case WUKONG_AI_EVENT_ASR_OK:
        {
            // s_ai_wakeup.state = AI_CHAT_THINK;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_THINK);
            __ai_wakeup_asr_result(event->type, event->data);
        }
        break;
            
        case WUKONG_AI_EVENT_TTS_PRE:
        {
            // s_ai_wakeup.state = AI_CHAT_SPEAK;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_SPEAK);
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
            __ai_wakeup_text_stream(event->type, event->data);
        }
        break;

        case WUKONG_AI_EVENT_EMOTION:
        case WUKONG_AI_EVENT_LLM_EMOTION:
        {
            __ai_wakeup_emition(event->type, event->data);
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
        case WUKONG_AI_EVENT_EXIT:
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_IDLE);
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
            if(s_ai_wakeup.wakeup_stat)
            {
                // s_ai_wakeup.state = AI_CHAT_LISTEN;
                MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_LISTEN);
            }
            else
            {
                // s_ai_wakeup.state = AI_CHAT_IDLE;
                MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_IDLE);
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

    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_wakeup_wakeup(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] wakeup"); 
    OPERATE_RET rt = OPRT_OK;

    wukong_audio_player_stop(AI_PLAYER_ALL);
    wukong_audio_input_reset();
    tuya_ai_agent_event(AI_EVENT_CHAT_BREAK, 0);

    wukong_audio_player_alert(AI_TOY_ALERT_TYPE_WAKEUP, FALSE);
    // s_ai_wakeup.state = AI_CHAT_LISTEN;
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_LISTEN);
    s_ai_wakeup.wakeup_stat = TRUE;

    return rt;
}

STATIC OPERATE_RET wukong_ai_wakeup_vad(VOID *data, INT_T len)
{
    if(!s_ai_wakeup.wakeup_stat)
    {
        return OPRT_OK;
    }

    TUYA_CHECK_NULL_RETURN(data, OPRT_INVALID_PARM);
    OPERATE_RET rt = OPRT_OK;

    WUKONG_AUDIO_VAD_FLAG_E vad_flag = (WUKONG_AUDIO_VAD_FLAG_E)data;
    TAL_PR_DEBUG("[====ai_wakeup] vad: [%d]", vad_flag); 
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
        // s_ai_wakeup.state = AI_CHAT_UPLOAD;
    }

    return rt;
}

STATIC OPERATE_RET wukong_ai_wakeup_client_run(VOID_T *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] client run");
    // s_ai_wakeup.state = AI_CHAT_IDLE;
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_IDLE);
    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_wakeup_key_cb(VOID *data, INT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    PUSH_KEY_TYPE_E event = *(PUSH_KEY_TYPE_E *)data;
    TAL_PR_DEBUG("[====ai_wakeup] key: %d", event);
    switch (event) 
    {        
        case NORMAL_KEY:
        {
            wukong_audio_player_stop(AI_PLAYER_ALL);
            wukong_audio_input_reset();
            tuya_ai_agent_event(AI_EVENT_CHAT_BREAK, 0);

            wukong_audio_player_alert(AI_TOY_ALERT_TYPE_WAKEUP, FALSE);
            // s_ai_wakeup.state = AI_CHAT_LISTEN;
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_LISTEN);
            s_ai_wakeup.wakeup_stat = TRUE;

        } 
        break;  

        case SEQ_KEY:
        {
            ;
        }
        break;

        case LONG_KEY: 
        {
            ;
        }
        break;   

        case RELEASE_KEY: 
        {
            ;
        }
        break;

        default:
        break;
    }
    
    return rt;
}

STATIC OPERATE_RET wukong_ai_wakeup_notify_idle_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_wakeup] client run");
    OPERATE_RET rt = OPRT_OK;

    // s_ai_wakeup.state = AI_CHAT_IDLE;
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_IDLE);

    return rt;
}

OPERATE_RET ai_wakeup_register(AI_CHAT_MODE_HANDLE_T **cb)
{
    OPERATE_RET rt = OPRT_OK;

    s_ai_wakeup_cb.ai_handle_init      = wukong_ai_wakeup_int_cb;
    s_ai_wakeup_cb.ai_handle_deinit    = wukong_ai_wakeup_deint_cb;
    s_ai_wakeup_cb.ai_handle_key       = wukong_ai_wakeup_key_cb;
    s_ai_wakeup_cb.ai_handle_task      = wukong_ai_wakeup_task_cb;
    s_ai_wakeup_cb.ai_handle_event     = wukong_ai_wakeup_event_cb;
    s_ai_wakeup_cb.ai_handle_wakeup    = wukong_ai_wakeup_wakeup;
    s_ai_wakeup_cb.ai_handle_vad       = wukong_ai_wakeup_vad;
    s_ai_wakeup_cb.ai_handle_client    = wukong_ai_wakeup_client_run;

    s_ai_wakeup_cb.ai_notify_idle      = wukong_ai_wakeup_notify_idle_cb;
    *cb = &s_ai_wakeup_cb;
    return rt;
}
#endif
