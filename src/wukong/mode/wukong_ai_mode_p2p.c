#include "wukong_ai_mode_p2p.h"
#if defined(ENABLE_AI_MODE_P2P) && (ENABLE_AI_MODE_P2P == 1)
#include "tuya_p2p_app.h"

STATIC AI_CHAT_MODE_HANDLE_T s_ai_p2p_cb = {0};
STATIC AI_P2P_PARAM_T s_ai_p2p = {0};

STATIC OPERATE_RET wukong_ai_p2p_int_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_p2p] init");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_toy_led_on();

    //set vad mode
    wukong_audio_input_wakeup_mode_set(WUKONG_AUDIO_VAD_MANIAL);

    //disenable kws
    wukong_kws_disable();

    MODE_STATE_CHANGE(AI_TRIGGER_MODE_P2P, s_ai_p2p.state, AI_CHAT_IDLE);

    s_ai_p2p.wakeup_stat = TRUE;

    wukong_audio_input_wakeup_set(TRUE);
#ifdef ENABLE_TUYA_UI
    AI_TRIGGER_MODE_E trigger_mode = AI_TRIGGER_MODE_P2P;
    tuya_ai_display_msg(&trigger_mode, 1, TY_DISPLAY_TP_CHAT_MODE);
#endif

    return rt;
}

STATIC OPERATE_RET wukong_ai_p2p_deint_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_p2p] deinit");
    OPERATE_RET rt = OPRT_OK;

    tuya_ai_input_stop();
    return rt;
}

STATIC OPERATE_RET wukong_ai_p2p_task_cb(VOID *data, INT_T len)
{
    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_p2p_event_cb(VOID *data, INT_T len)
{
    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_p2p_wakeup(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_p2p] wakeup"); 
    OPERATE_RET rt = OPRT_OK;
    return rt;
}

STATIC OPERATE_RET wukong_ai_p2p_vad(VOID *data, INT_T len)
{
    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_p2p_client_run(VOID_T *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_p2p] client run");
    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_p2p_key_cb(VOID *data, INT_T len)
{
    OPERATE_RET rt = OPRT_OK;
    PUSH_KEY_TYPE_E event = *(PUSH_KEY_TYPE_E *)data;
    TAL_PR_DEBUG("[====ai_p2p] key: %d", event);
    switch (event) 
    {        
        case NORMAL_KEY:
        {
            wukong_audio_player_stop(AI_PLAYER_ALL);
            wukong_audio_input_reset();
            tuya_ai_agent_event(AI_EVENT_CHAT_BREAK, 0);
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_P2P, s_ai_p2p.state, AI_CHAT_IDLE);
            s_ai_p2p.wakeup_stat = FALSE;

        } 
        break;  

        case SEQ_KEY:
        {
            ;
        }
        break;

        case LONG_KEY: 
        {
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_P2P, s_ai_p2p.state, AI_CHAT_LISTEN);
            s_ai_p2p.wakeup_stat = TRUE;
        }
        break;   

        case RELEASE_KEY: 
        {
            MODE_STATE_CHANGE(AI_TRIGGER_MODE_P2P, s_ai_p2p.state, AI_CHAT_UPLOAD);
        }
        break;

        default:
        break;
    }
    
    return rt;
}

STATIC OPERATE_RET wukong_ai_p2p_notify_idle_cb(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_p2p] notify idle");
    return OPRT_OK;
}

STATIC OPERATE_RET wukong_ai_p2p_handle_audio_input(VOID *data, INT_T len)
{
    TAL_PR_DEBUG("[====ai_p2p] handle audio input");
    return tuya_ipc_app_audio_frame_put(data, len);
}

OPERATE_RET ai_p2p_register(AI_CHAT_MODE_HANDLE_T **cb)
{
    OPERATE_RET rt = OPRT_OK;

    s_ai_p2p_cb.ai_handle_init      = wukong_ai_p2p_int_cb;
    s_ai_p2p_cb.ai_handle_deinit    = wukong_ai_p2p_deint_cb;
    s_ai_p2p_cb.ai_handle_key       = wukong_ai_p2p_key_cb;
    s_ai_p2p_cb.ai_handle_task      = wukong_ai_p2p_task_cb;
    s_ai_p2p_cb.ai_handle_event     = wukong_ai_p2p_event_cb;
    s_ai_p2p_cb.ai_handle_wakeup    = wukong_ai_p2p_wakeup;
    s_ai_p2p_cb.ai_handle_vad       = wukong_ai_p2p_vad;
    s_ai_p2p_cb.ai_handle_client    = wukong_ai_p2p_client_run;

    s_ai_p2p_cb.ai_notify_idle      = wukong_ai_p2p_notify_idle_cb;
    s_ai_p2p_cb.ai_handle_audio_input = wukong_ai_p2p_handle_audio_input;
    *cb = &s_ai_p2p_cb;
    return rt;
}
#endif