#ifndef __AI_CHAT_MODE_H__
#define __AI_CHAT_MODE_H__
#include "tuya_cloud_types.h"
#include "uni_log.h"
#include "tuya_device_cfg.h"
#include "tuya_app_config.h"
#include "tal_memory.h"
#include "tuya_key.h"
#include "tal_mutex.h"
#include "wukong_ai_agent.h"
#include "wukong_audio_input.h"
#include "tuya_ai_toy_led.h"
#include "tuya_app_config.h"
#include "wukong_audio_player.h"
#include "tuya_ai_agent.h"
#include "tuya_ai_protocol.h"
#include "base_event.h"
#include "wukong_ai_skills.h"
#include "skill_emotion.h"
#include "tuya_device_cfg.h"
#include "wukong_audio_aec_vad.h"
#ifdef ENABLE_TUYA_UI
#include "tuya_ai_display.h"
#endif

#define AI_AGENT_SCODE_DEFAULT ""

typedef enum {
    AI_TRIGGER_MODE_HOLD,        // 长按触发模式
    AI_TRIGGER_MODE_ONE_SHOT,    // 单次按键，回合制对话模式
    AI_TRIGGER_MODE_WAKEUP,      // 关键词唤醒模式
    AI_TRIGGER_MODE_FREE,        // 关键词唤醒和自由对话模式
    AI_TRIGGER_MODE_P2P,         // P2P模式
    AI_TRIGGER_MODE_TRANSLATE,   // 翻译模式
    AI_TRIGGER_MODE_PICTURE_GEN, // 生图模式
    AI_TRIGGER_MODE_MAX,
} AI_TRIGGER_MODE_E;

typedef enum {
    AI_CHAT_INIT,
    AI_CHAT_IDLE,
    AI_CHAT_LISTEN,
    AI_CHAT_UPLOAD,
    AI_CHAT_THINK,
    AI_CHAT_SPEAK,
    AI_CHAT_INVALID,
} AI_CHAT_STATE_E;

typedef struct 
{
    OPERATE_RET (*ai_handle_init)   (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_deinit) (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_key)    (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_task)   (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_event)  (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_wakeup) (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_vad)    (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_client) (VOID *data, INT_T len);
    OPERATE_RET (*ai_notify_idle)   (VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_audio_input)(VOID *data, INT_T len);//处理本地音频采集数据，适用于P2P等模式
}AI_CHAT_MODE_HANDLE_T;

typedef struct
{
    BOOL_T enabled;
    MUTEX_HANDLE mutex;
    AI_TRIGGER_MODE_E trigger_mode;
    AI_CHAT_MODE_HANDLE_T *handle;
}AI_MODE_SWITCH_T;

extern CHAR_T *_mode_str[];
extern CHAR_T *_state_str[];
#define MODE_STATE_CHANGE(_mode, _old, _new) \
do { \
    PR_DEBUG("mode %s state change form %s to %s", _mode_str[_mode], _state_str[_old], _state_str[_new]);\
    _old = _new; \
}while (0)


OPERATE_RET wukong_ai_mode_init();
OPERATE_RET wukong_ai_mode_switch(AI_TRIGGER_MODE_E cur_mode);
OPERATE_RET wukong_ai_mode_switch_to(AI_TRIGGER_MODE_E mode);
OPERATE_RET wukong_ai_handle_init(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_deinit(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_key(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_task(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_event(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_wakeup(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_vad(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_client(VOID *data, INT_T len);
OPERATE_RET wukong_ai_notify_idle(VOID *data, INT_T len);
OPERATE_RET wukong_ai_handle_audio_input(VOID *data, INT_T len);

#endif  // __AI_CHAT_MODE_H__