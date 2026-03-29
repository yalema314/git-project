#ifndef __AI_MODE_TRANSLATE_H__
#define __AI_MODE_TRANSLATE_H__

#include "wukong_ai_mode.h"

typedef struct
{
    BOOL_T wakeup_stat;
    AI_CHAT_STATE_E state;
}AI_TRANSLATE_PARAM_T;

OPERATE_RET ai_translate_register(AI_CHAT_MODE_HANDLE_T **cb);

#endif  // __AI_MODE_TRANSLATE_H__