
# 模式模块

## 概述

模式模块管理不同的 AI 对话触发模式，为实现各种交互模式提供灵活的框架。每个模式实现一个状态机来处理对话流程。

## 目录结构

```
mode/
├── wukong_ai_mode.h/c              # 模式管理器（核心）
├── wukong_ai_mode_free.h/c          # 自由对话模式
├── wukong_ai_mode_hold.h/c          # 按住按键触发模式
├── wukong_ai_mode_oneshot.h/c       # 单次按键触发模式
├── wukong_ai_mode_wakeup.h/c        # 关键词唤醒模式
├── wukong_ai_mode_p2p.h/c           # P2P 通信模式
└── wukong_ai_mode_translate.h/c      # 翻译模式
```

## 支持的模式

### 1. 按住模式 (`wukong_ai_mode_hold`)

**触发方式**: 长按按键  
**使用场景**: 按键通话式对话  
**特性**:
- 手动 VAD 模式（按键控制录音）
- KWS 禁用
- 服务器 VAD 禁用

### 2. 单次模式 (`wukong_ai_mode_oneshot`)

**触发方式**: 单次按键  
**使用场景**: 回合制对话  
**特性**:
- 手动 VAD 模式
- KWS 禁用
- 服务器 VAD 禁用

### 3. 唤醒模式 (`wukong_ai_mode_wakeup`)

**触发方式**: 关键词唤醒  
**使用场景**: 免提语音交互  
**特性**:
- 自动 VAD 模式
- KWS 启用
- 服务器 VAD 启用

### 4. 自由模式 (`wukong_ai_mode_free`)

**触发方式**: 关键词唤醒 + 连续对话  
**使用场景**: 自然对话流程  
**特性**:
- 自动 VAD 模式
- KWS 启用
- 服务器 VAD 启用
- 支持连续对话

### 5. P2P 模式 (`wukong_ai_mode_p2p`)

**触发方式**: P2P 通信  
**使用场景**: 直接点对点音频通信  
**特性**:
- 手动 VAD 模式
- KWS 禁用
- 直接音频流传输

### 6. 翻译模式 (`wukong_ai_mode_translate`)

**触发方式**: 关键词唤醒  
**使用场景**: 实时语言翻译  
**特性**:
- 自动 VAD 模式
- KWS 启用
- ASR-LLM-TTS 工作流
- 语言切换支持

## 状态机

所有模式遵循共同的状态机：

```
INIT → IDLE → LISTEN → UPLOAD → THINK → SPEAK
        ↑       ↑                         ↓
        └───────└─────────────────────────┘
```

### 状态描述

- **INIT**: 初始化状态
- **IDLE**: 空闲状态，等待触发
- **LISTEN**: 监听用户输入
- **UPLOAD**: 上传音频到云端
- **THINK**: AI 处理（云端思考）
- **SPEAK**: 播放 AI 回复（TTS）

## 核心 API

### 模式管理

```c
/**
 * @brief 初始化模式系统
 * 
 * 注册所有启用的模式并初始化默认模式。
 * 
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_mode_init(VOID);

/**
 * @brief 切换到下一个启用的模式
 * 
 * @param[in] cur_mode 当前模式
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_mode_switch(AI_TRIGGER_MODE_E cur_mode);

/**
 * @brief 切换到指定模式
 * 
 * @param[in] mode 目标模式
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_mode_switch_to(AI_TRIGGER_MODE_E mode);
```

### 事件处理器

```c
/**
 * @brief 处理模式初始化
 * 
 * @param[in] data 初始化数据
 * @param[in] len 数据长度
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_handle_init(VOID *data, INT_T len);

/**
 * @brief 处理按键事件
 * 
 * @param[in] data 按键事件数据
 * @param[in] len 数据长度
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_handle_key(VOID *data, INT_T len);

/**
 * @brief 处理 AI 事件
 * 
 * @param[in] data 事件数据
 * @param[in] len 数据长度
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_handle_event(VOID *data, INT_T len);

/**
 * @brief 处理唤醒事件
 * 
 * @param[in] data 唤醒数据
 * @param[in] len 数据长度
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_handle_wakeup(VOID *data, INT_T len);

/**
 * @brief 处理 VAD 事件
 * 
 * @param[in] data VAD 标志
 * @param[in] len 数据长度
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_ai_handle_vad(VOID *data, INT_T len);
```

## 模式处理器接口

每个模式实现以下接口：

```c
typedef struct {
    OPERATE_RET (*ai_handle_init)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_deinit)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_key)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_task)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_event)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_wakeup)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_vad)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_client)(VOID *data, INT_T len);
    OPERATE_RET (*ai_notify_idle)(VOID *data, INT_T len);
    OPERATE_RET (*ai_handle_audio_input)(VOID *data, INT_T len);
} AI_CHAT_MODE_HANDLE_T;
```

## 使用示例

### 基础初始化

```c
#include "wukong_ai_mode.h"

/* 初始化模式系统 */
OPERATE_RET init_modes(VOID)
{
    OPERATE_RET rt = OPRT_OK;
    
    /* 初始化模式管理器 */
    /* 这将注册所有启用的模式并启动默认模式 */
    rt = wukong_ai_mode_init();
    if (rt != OPRT_OK) {
        printf("初始化模式失败: %d\n", rt);
        return rt;
    }
    
    return rt;
}
```

### 切换模式

```c
/* 切换到唤醒模式 */
wukong_ai_mode_switch_to(AI_TRIGGER_MODE_WAKEUP);

/* 切换到下一个启用的模式 */
AI_TRIGGER_MODE_E current = tuya_ai_toy_trigger_mode_get();
wukong_ai_mode_switch(current);
```

### 处理事件

```c
/* 处理按键事件 */
PUSH_KEY_TYPE_E key_event = NORMAL_KEY;
wukong_ai_handle_key(&key_event, sizeof(key_event));

/* 处理 AI 事件 */
WUKONG_AI_EVENT_T event = {
    .type = WUKONG_AI_EVENT_ASR_OK,
    .data = asr_result,
};
wukong_ai_handle_event(&event, sizeof(event));

/* 处理唤醒事件 */
WUKONG_KWS_INDEX_E keyword = WUKONG_KWS_NIHAOTUYA;
wukong_ai_handle_wakeup(&keyword, sizeof(keyword));

/* 处理 VAD 事件 */
WUKONG_AUDIO_VAD_FLAG_E vad_flag = WUKONG_AUDIO_VAD_START;
wukong_ai_handle_vad(&vad_flag, sizeof(vad_flag));
```

## 创建新模式

### 步骤 1: 定义模式结构

```c
/* wukong_ai_mode_my_mode.h */
#ifndef __WUKONG_AI_MODE_MY_MODE_H__
#define __WUKONG_AI_MODE_MY_MODE_H__

#include "wukong_ai_mode.h"

typedef struct {
    BOOL_T wakeup_stat;
    AI_CHAT_STATE_E state;
} AI_MY_MODE_PARAM_T;

OPERATE_RET ai_my_mode_register(AI_CHAT_MODE_HANDLE_T **cb);

#endif
```

### 步骤 2: 实现模式处理器

```c
/* wukong_ai_mode_my_mode.c */
#include "wukong_ai_mode_my_mode.h"

STATIC AI_CHAT_MODE_HANDLE_T s_ai_my_mode_cb = {0};
STATIC AI_MY_MODE_PARAM_T s_ai_my_mode = {0};
STATIC AI_CHAT_STATE_E s_ai_cur_state = AI_CHAT_INVALID;

/* 状态回调 */
STATIC OPERATE_RET __ai_my_mode_idle_cb(VOID *data, INT_T len)
{
    tuya_ai_toy_led_off();
    wukong_audio_input_wakeup_set(FALSE);
    return OPRT_OK;
}

STATIC OPERATE_RET __ai_my_mode_listen_cb(VOID *data, INT_T len)
{
    tuya_ai_toy_led_flash(500);
    wukong_audio_input_wakeup_set(TRUE);
    return OPRT_OK;
}

/* ... 实现其他状态回调 ... */

/* 事件处理器 */
STATIC OPERATE_RET wukong_ai_my_mode_event_cb(VOID *data, INT_T len)
{
    WUKONG_AI_EVENT_T *event = (WUKONG_AI_EVENT_T *)data;
    
    switch (event->type) {
    case WUKONG_AI_EVENT_ASR_OK:
        MODE_STATE_CHANGE(AI_TRIGGER_MODE_MY_MODE, s_ai_my_mode.state, AI_CHAT_THINK);
        break;
        
    case WUKONG_AI_EVENT_TTS_PRE:
        MODE_STATE_CHANGE(AI_TRIGGER_MODE_MY_MODE, s_ai_my_mode.state, AI_CHAT_SPEAK);
        break;
        
    /* ... 处理其他事件 ... */
    }
    
    return OPRT_OK;
}

/* 初始化处理器 */
STATIC OPERATE_RET wukong_ai_my_mode_init_cb(VOID *data, INT_T len)
{
    wukong_audio_input_wakeup_mode_set(WUKONG_AUDIO_VAD_AUTO);
    wukong_kws_enable();
    MODE_STATE_CHANGE(AI_TRIGGER_MODE_MY_MODE, s_ai_my_mode.state, AI_CHAT_IDLE);
    return OPRT_OK;
}

/* 注册模式 */
OPERATE_RET ai_my_mode_register(AI_CHAT_MODE_HANDLE_T **cb)
{
    s_ai_my_mode_cb.ai_handle_init = wukong_ai_my_mode_init_cb;
    s_ai_my_mode_cb.ai_handle_deinit = wukong_ai_my_mode_deinit_cb;
    s_ai_my_mode_cb.ai_handle_key = wukong_ai_my_mode_key_cb;
    s_ai_my_mode_cb.ai_handle_task = wukong_ai_my_mode_task_cb;
    s_ai_my_mode_cb.ai_handle_event = wukong_ai_my_mode_event_cb;
    s_ai_my_mode_cb.ai_handle_wakeup = wukong_ai_my_mode_wakeup;
    s_ai_my_mode_cb.ai_handle_vad = wukong_ai_my_mode_vad;
    s_ai_my_mode_cb.ai_handle_client = wukong_ai_my_mode_client_run;
    s_ai_my_mode_cb.ai_notify_idle = wukong_ai_my_mode_notify_idle_cb;
    
    *cb = &s_ai_my_mode_cb;
    return OPRT_OK;
}
```

### 步骤 3: 注册模式

```c
/* 在 wukong_ai_mode.c 中 */
#if defined(ENABLE_AI_MODE_MY_MODE) && (ENABLE_AI_MODE_MY_MODE == 1)
    s_ai_mode_map[AI_TRIGGER_MODE_MY_MODE].enabled = TRUE;
    tal_mutex_create_init(&s_ai_mode_map[AI_TRIGGER_MODE_MY_MODE].mutex);
    s_ai_mode_map[AI_TRIGGER_MODE_MY_MODE].trigger_mode = AI_TRIGGER_MODE_MY_MODE;
    ai_my_mode_register(&s_ai_mode_map[AI_TRIGGER_MODE_MY_MODE].handle);
#else
    s_ai_mode_map[AI_TRIGGER_MODE_MY_MODE].enabled = FALSE;
    s_ai_mode_map[AI_TRIGGER_MODE_MY_MODE].trigger_mode = AI_TRIGGER_MODE_MY_MODE;
    s_ai_mode_map[AI_TRIGGER_MODE_MY_MODE].handle = NULL;
#endif
```

### 步骤 4: 添加模式枚举

```c
/* 在 wukong_ai_mode.h 中 */
typedef enum {
    AI_TRIGGER_MODE_HOLD,
    AI_TRIGGER_MODE_ONE_SHOT,
    AI_TRIGGER_MODE_WAKEUP,
    AI_TRIGGER_MODE_FREE,
    AI_TRIGGER_MODE_P2P,
    AI_TRIGGER_MODE_TRANSLATE,
    AI_TRIGGER_MODE_MY_MODE,      /* 添加新模式 */
    AI_TRIGGER_MODE_MAX,
} AI_TRIGGER_MODE_E;
```

## 状态变更宏

使用 `MODE_STATE_CHANGE` 宏进行状态转换：

```c
#define MODE_STATE_CHANGE(_mode, _old, _new) \
do { \
    PR_DEBUG("模式 %s 状态从 %s 变更为 %s", \
             _mode_str[_mode], _state_str[_old], _state_str[_new]); \
    _old = _new; \
} while (0)

/* 使用 */
MODE_STATE_CHANGE(AI_TRIGGER_MODE_WAKEUP, s_ai_wakeup.state, AI_CHAT_LISTEN);
```