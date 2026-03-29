#ifndef __WUKONG_AI_PARSE_H__
#define __WUKONG_AI_PARSE_H__

#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_ai_output.h"
#include "ty_cJSON.h"
#include "smart_frame.h"
#include "wukong_ai_agent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CHAR_T *data;
    UINT16_T datalen;
    UINT32_T timeindex;
} WUKONG_AI_TEXT_T;

// skills:
// {"bizId":"asr-1741763201372","bizType":"ASR","eof":1,"data":{"text":"这是ASR文本！"}}
// {"bizId":"nlg-1741763173046","bizType":"NLG","eof":0,"data":{"content":"这是NLG响应文本！","appendMode":"append","finish":false}}
// {"bizId":"skill-........emo","bizType":"SKILL","eof":1,"data":{"code":"emo","skillContent":{"emotion": ["sad", "happy"], "text":["😢","😀"]}}}
// {"bizId":"skill-......music","bizType":"SKILL","eof":1,"data":{"code":"music","skillContent":{"playList": [item1, item2]}}}

VOID wukong_ai_text_process(AI_TEXT_TYPE_E type, ty_cJSON *root, BOOL_T eof);
VOID wukong_skill_notify_chat_break();


#ifdef __cplusplus
}
#endif

#endif  /* __WUKONG_AI_PARSE_H__ */
