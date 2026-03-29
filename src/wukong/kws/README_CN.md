# KWS（关键词唤醒）模块

## 概述

KWS 模块为 Wukong AI 系统提供关键词唤醒检测功能。它支持多个预定义关键词，并可以扩展用户自定义关键词。

## 目录结构

```
kws/
├── wukong_kws.h/c              # KWS 接口包装器
├── sndx/                        # SNDX 唤醒引擎
│   ├── sndx_kws.h/c
│   └── [库文件]
├── tutuclear/                   # TuTuClear 唤醒引擎
│   ├── tutuclear_kws.h/c
│   └── [库文件]
└── uart/                        # UART KWS 实现
    └── wukong_kws_uart.h/c
```

## 特性

- **多唤醒引擎**: 支持 SNDX 和 TuTuClear 引擎
- **预定义关键词**: 内置支持常用唤醒短语
- **用户自定义关键词**: 支持自定义唤醒词
- **事件发布**: 检测到关键词时发布唤醒事件
- **集成**: 与音频输入和模式模块无缝协作

## 支持的关键词

### SNDX 引擎
- **libsndxasr-nihaotuya.a**: "你好涂鸦" 
- **libsndxasr-hey-tuya.a**: "Hey Tuya" 
- **libsndxasr-hey-smart-life.a**: "Hey SmartLife/SmartLife"

### TuTuClear 引擎

- **libtutuClear_wakeup_nihaotuya_chinese_20251227_200KB_model.a**: "你好涂鸦"
- **libtutuClear_wakeup_xiaozhitongxue_chinese_20251227_200KB_model.a**: "小智同学" 
- **libtutuClear_wakeup_heytuya_english_20251227_730KB_model.a**: "Hey Tuya" 
- **libtutuClear_wakeup_nihaotuya_xiaozhitongxue_heytuya_hituya_20251024_small_model.a**: "你好涂鸦","小智同学" ,"Hey Tuya"  三合一模型

### 用户自定义关键词

- `WUKONG_KWS_UDF1`: 用户自定义关键词 1
- `WUKONG_KWS_UDF2`: 用户自定义关键词 2
- `WUKONG_KWS_UDF3`: 用户自定义关键词 3

## API 参考

### 初始化

```c
/**
 * @brief KWS 配置结构
 */
typedef struct {
    OPERATE_RET (*create)(WUKONG_KWS_CTX_T *ctx);
    OPERATE_RET (*destroy)(WUKONG_KWS_CTX_T *ctx);
    OPERATE_RET (*process)(WUKONG_KWS_CTX_T *ctx, INT16_T *data, UINT16_T len);
    OPERATE_RET (*enable)(WUKONG_KWS_CTX_T *ctx, WUKONG_KWS_INDEX_E index);
    OPERATE_RET (*disable)(WUKONG_KWS_CTX_T *ctx);
} WUKONG_KWS_CFG_T;

/**
 * @brief 初始化 KWS 模块
 * 
 * @param[in] cfg 包含回调函数的 KWS 配置
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_kws_init(WUKONG_KWS_CFG_T *cfg);

/**
 * @brief 反初始化 KWS 模块
 * 
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_kws_deinit(VOID);
```

### 控制

```c
/**
 * @brief 启用 KWS 检测
 * 
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_kws_enable(VOID);

/**
 * @brief 禁用 KWS 检测
 * 
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_kws_disable(VOID);

/**
 * @brief 处理音频数据
 * 
 * 此函数应使用来自音频输入模块的音频帧调用。
 * 
 * @param[in] data 音频数据缓冲区（16 位 PCM）
 * @param[in] len 数据长度（字节）
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_kws_process(UINT8_T *data, UINT16_T len);
```

### 关键词索引

```c
typedef enum {
    WUKONG_KWS_NIHAOTUYA = 1,           // "你好涂鸦"
    WUKONG_KWS_XIAOZHITONGXUE = 2,      // "小智同学"
    WUKONG_KWS_HEYTUYA = 3,             // "hey-tuya"
    WUKONG_KWS_SMARTLIFE = 4,           // "smartlife"
    WUKONG_KWS_ZHINENGGUANJIA = 5,      // "智能管家"
    WUKONG_KWS_UDF1 = 6,                // 用户自定义 1
    WUKONG_KWS_UDF2 = 7,                // 用户自定义 2
    WUKONG_KWS_UDF3 = 8,                // 用户自定义 3
} WUKONG_KWS_INDEX_E;
```

## 事件系统

当检测到关键词时，KWS 模块发布一个事件：

- **事件名称**: `EVENT_WUKONG_KWS_WAKEUP`
- **事件数据**: `WUKONG_KWS_INDEX_E`（关键词索引）

### 事件订阅示例

```c
VOID on_kws_wakeup(VOID *data, UINT_T len)
{
    WUKONG_KWS_INDEX_E keyword = *(WUKONG_KWS_INDEX_E *)data;
    printf("检测到唤醒关键词: %d\n", keyword);
    
    /* 处理唤醒事件 */
    wukong_ai_handle_wakeup(&keyword, sizeof(keyword));
}

/* 订阅唤醒事件 */
ty_subscribe_event(EVENT_WUKONG_KWS_WAKEUP, "my_module", on_kws_wakeup, SUBSCRIBE_TYPE_NORMAL);
```

## 使用示例

### 基础初始化

```c
#include "wukong_kws.h"
wukong_kws_init(&kws_cfg);
wukong_kws_enable();
```

## 音频要求

- **采样率**: 16kHz
- **采样格式**: 16 位 PCM
- **声道**: 单声道
- **帧大小**: 通常为 320 个采样（20ms）


## 自定义

### 添加自定义关键词

1. 自己训练，或者寻求专业的方案商，或者寻求涂鸦协助，训练一个关键词模型
2. 将关键词索引添加到 `WUKONG_KWS_INDEX_E` 枚举
3. 将模型修改为`lib<yourkws>.a`，并拷贝到./libs/目录下
4. 参考切换引擎，将默认的唤醒词模型修改为你自定义的关键词模型
5. 使用新的关键词索引发布唤醒事件

### 切换引擎

KWS 模块使用基于回调的架构，允许轻松切换引擎：

```c
/* 切换到 SNDX */
WUKONG_KWS_CFG_T kws_cfg = {
    .create = sndx_kws_create,
    .destroy = sndx_kws_destroy,
    .process = sndx_kws_process,
    .enable = sndx_kws_enable,
    .disable = sndx_kws_disable,
};
wukong_kws_init(&kws_cfg);
wukong_kws_enable();

/* 切换到 TuTuClear */
WUKONG_KWS_CFG_T kws_cfg = {
    .create = tutuclear_kws_create,
    .destroy = tutuclear_kws_destroy,
    .process = tutuclear_kws_process,
    .enable = tutuclear_kws_enable,
    .disable = tutuclear_kws_disable,
};
wukong_kws_init(&kws_cfg);
wukong_kws_enable();

/* 切换到 自定义 */
WUKONG_KWS_CFG_T kws_cfg = {
    .create = your_kws_create,
    .destroy = your_kws_destroy,
    .process = your_kws_process,
    .enable = your_kws_enable,
    .disable = your_kws_disable,
};
wukong_kws_init(&kws_cfg);
wukong_kws_enable();
```
