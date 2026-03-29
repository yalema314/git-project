#ifndef __AI_MODE_FREE_H__
#define __AI_MODE_FREE_H__

#include "wukong_ai_mode.h"

typedef struct
{
    BOOL_T wakeup_stat;
    AI_CHAT_STATE_E state;
}AI_FREE_PARAM_T;

OPERATE_RET ai_free_register(AI_CHAT_MODE_HANDLE_T **cb);
BOOL_T ai_free_wakeup_is_active(VOID);


#endif  // __AI_MODE_FREE_H__
