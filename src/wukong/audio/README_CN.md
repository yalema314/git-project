# 音频模块

## 概述

音频模块为 Wukong AI 系统提供全面的音频处理能力，包括音频输入输出、AEC（声学回声消除）、VAD（语音活动检测）和音频播放管理。

## 目录结构

```
audio/
├── wukong_audio_input.h/c        # 音频输入接口（包装器）
├── wukong_audio_output.h/c       # 音频输出接口（包装器）
├── wukong_audio_player.h/c        # 音频播放器（TTS、音乐、提示音）
├── wukong_audio_aec_vad.h/c      # AEC 和 VAD 处理
├── board/                         # 板级特定实现
│   ├── wukong_audio_input_board.c
│   └── wukong_audio_output_board.c
└── uart/                          # UART 音频实现
    ├── wukong_audio_input_uart.c
    └── wukong_audio_output_uart.c
```

## 核心组件

### 1. 音频输入 (`wukong_audio_input`)

为来自不同源（板级或 UART）的音频输入提供统一接口。

#### 特性

- **多输入源**: 支持板级音频和 UART 音频输入
- **VAD 模式**: 手动（按键触发）和自动（语音检测）
- **生产者模式**: 可插拔的音频输入生产者

#### 核心 API

```c
/**
 * @brief 初始化音频输入
 * 
 * @param[in] cfg 音频输入配置
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_input_init(WUKONG_AUDIO_INPUT_CFG_T *cfg);

/**
 * @brief 启动音频输入
 * 
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_input_start(VOID);

/**
 * @brief 停止音频输入
 * 
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_input_stop(VOID);

/**
 * @brief 设置唤醒模式
 * 
 * @param[in] enable TRUE 启用唤醒，FALSE 禁用
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_input_wakeup_set(BOOL_T enable);

/**
 * @brief 设置 VAD 模式
 * 
 * @param[in] mode VAD 模式（手动或自动）
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_input_wakeup_mode_set(WUKONG_AUDIO_VAD_MODE_E mode);
```

#### 配置

```c
typedef struct {
    WUKONG_AUDIO_TYPE_E type;      // 板级或 UART
    UINT16_T timeout;
    union {
        WUKONG_BOARD_AUDIO_INPUT_CFG_T board;
        TDL_COMM_AUDIO_CFG_T uart;
    };
} WUKONG_AUDIO_INPUT_CFG_T;

typedef struct {
    TKL_AUDIO_SAMPLE_E sample_rate;    // 采样率
    TKL_AUDIO_DATABITS_E sample_bits;  // 采样位数
    TKL_AUDIO_CHANNEL_E channel;       // 声道配置
    TUYA_GPIO_NUM_E spk_io;            // 扬声器 GPIO
    TUYA_GPIO_LEVEL_E spk_io_level;    // GPIO 极性
    
    WUKONG_AUDIO_VAD_MODE_E vad_mode;  // VAD 模式
    UINT16_T vad_off_ms;               // VAD 补偿时间
    UINT16_T vad_active_ms;            // VAD 检测阈值
    UINT16_T slice_ms;                 // 音频切片时间
    
    WUKONG_AUDIO_OUTPUT output_cb;     // 数据回调
    VOID *user_data;                   // 用户数据
} WUKONG_BOARD_AUDIO_INPUT_CFG_T;
```

#### 使用示例

```c
/* 配置板级音频输入 */
WUKONG_AUDIO_INPUT_CFG_T input_cfg = {0};
input_cfg.type = WUKONG_AUDIO_USING_BOARD;
input_cfg.board.sample_rate = TKL_AUDIO_SAMPLE_16K;
input_cfg.board.sample_bits = TKL_AUDIO_DATABITS_16;
input_cfg.board.channel = TKL_AUDIO_CHANNEL_MONO;
input_cfg.board.vad_mode = WUKONG_AUDIO_VAD_AUTO;
input_cfg.board.vad_off_ms = 500;
input_cfg.board.vad_active_ms = 300;
input_cfg.board.slice_ms = 20;
input_cfg.board.output_cb = my_audio_callback;

/* 初始化并启动 */
wukong_audio_input_init(&input_cfg);
wukong_audio_input_start();
```

### 2. 音频输出 (`wukong_audio_output`)

为音频输出到不同目标提供统一接口。

#### 特性

- **多输出目标**: 支持板级音频和 UART 音频输出
- **消费者模式**: 可插拔的音频输出消费者

#### 核心 API

```c
/**
 * @brief 初始化音频输出
 * 
 * @param[in] cfg 音频输出配置
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_output_init(WUKONG_AUDIO_OUTPUT_CFG_T *cfg);

/**
 * @brief 启动音频输出
 * 
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_output_start(VOID);

/**
 * @brief 写入音频数据
 * 
 * @param[in] data 音频数据缓冲区
 * @param[in] datalen 数据长度（字节）
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_output_write(UINT8_T *data, UINT16_T datalen);

/**
 * @brief 设置音量
 * 
 * @param[in] volume 音量级别
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_output_set_vol(INT32_T volume);
```

### 3. 音频播放器 (`wukong_audio_player`)

管理 TTS、音乐和提示音的音频播放。

#### 特性

- **双播放器系统**: 前台（TTS）和后台（音乐）播放器
- **多格式支持**: 支持 MP3、WAV、Opus、Speex、OggOpus
- **音量混合**: TTS 播放时自动调整音乐音量
- **播放控制**: 播放、暂停、恢复、重播、停止操作

#### 核心 API

```c
/**
 * @brief 播放 TTS 流
 * 
 * @param[in] type 事件类型（START/DATA/STOP）
 * @param[in] codec 音频编解码格式
 * @param[in] data 音频数据缓冲区
 * @param[in] len 数据长度
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TYPE_E type, 
                                         AI_AUDIO_CODEC_E codec, 
                                         CHAR_T *data, INT_T len);

/**
 * @brief 播放音乐
 * 
 * @param[in] music 包含 URL 和配置的音乐结构
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_play_music(WUKONG_AI_MUSIC_T *music);

/**
 * @brief 播放提示音
 * 
 * @param[in] type 提示音类型
 * @param[in] cloud TRUE 表示来自云端，FALSE 表示本地
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_player_alert(TY_AI_TOY_ALERT_TYPE_E type, BOOL_T cloud);

/**
 * @brief 停止播放器
 * 
 * @param[in] player 播放器类型（FG/BG/ALL）
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_audio_player_stop(TY_AI_TOY_PLAYER_TYPE_E player);

/**
 * @brief 检查是否正在播放
 * 
 * @return 正在播放返回 TRUE，否则返回 FALSE
 */
BOOL_T wukong_audio_player_is_playing(VOID);
```

#### 支持的音频格式

- `AI_AUDIO_CODEC_MP3`: MP3 格式
- `AI_AUDIO_CODEC_WAV`: WAV 格式
- `AI_AUDIO_CODEC_OPUS`: Opus 格式
- `AI_AUDIO_CODEC_SPEEX`: Speex 格式
- `AI_AUDIO_CODEC_OGGOPUS`: OggOpus 格式

#### 使用示例

```c
/* 播放 TTS 流 */
wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TTS_START, 
                            AI_AUDIO_CODEC_MP3, 
                            audio_data, audio_len);

/* 播放音乐 */
WUKONG_AI_MUSIC_T music = {0};
music.action = "play";
music.src_array[0].url = "http://example.com/music.mp3";
music.src_array[0].format = "mp3";
music.src_count = 1;
wukong_audio_play_music(&music);

/* 播放提示音 */
wukong_audio_player_alert(AI_TOY_ALERT_TYPE_WAKEUP, FALSE);

/* 停止所有播放 */
wukong_audio_player_stop(AI_PLAYER_ALL);
```

### 4. AEC/VAD (`wukong_audio_aec_vad`)

提供声学回声消除和语音活动检测。

#### 核心 API

```c
/**
 * @brief 初始化 AEC/VAD
 * 
 * @param[in] sample_rate 音频采样率
 * @param[in] frame_size 帧大小（采样数）
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_aec_vad_init(UINT16_T sample_rate, UINT16_T frame_size);

/**
 * @brief 处理音频帧
 * 
 * @param[in] mic_data 麦克风数据
 * @param[in] ref_data 参考（扬声器）数据
 * @param[out] out_data 处理后的输出数据
 * @param[in] frame_size 帧大小（采样数）
 * @return VAD 标志（START/END/NONE）
 */
WUKONG_AUDIO_VAD_FLAG_E wukong_aec_vad_process(INT16_T *mic_data, 
                                               INT16_T *ref_data, 
                                               INT16_T *out_data, 
                                               UINT16_T frame_size);

/**
 * @brief 设置 VAD 阈值
 * 
 * @param[in] threshold VAD 阈值级别
 * @return 成功返回 OPRT_OK，否则返回错误码
 */
OPERATE_RET wukong_vad_set_threshold(WUKONG_AUDIO_VAD_THRESHOLD_E threshold);
```

#### VAD 阈值

- `WUKONG_AUDIO_VAD_LOW`: 低阈值（更敏感）
- `WUKONG_AUDIO_VAD_MID`: 中等阈值
- `WUKONG_AUDIO_VAD_HIGH`: 高阈值（较不敏感）
- 阈值对于的数值可以按需调整，哦那个-99到0，阈值越小越灵敏

#### 使用示例

```c
/* 初始化 AEC/VAD */
wukong_aec_vad_init(16000, 320);  // 16kHz，每帧 320 个采样

/* 处理音频帧 */
INT16_T mic_data[320];
INT16_T ref_data[320];
INT16_T out_data[320];
WUKONG_AUDIO_VAD_FLAG_E vad_flag = wukong_aec_vad_process(mic_data, ref_data, out_data, 320);

if (vad_flag == WUKONG_AUDIO_VAD_START) {
    printf("检测到语音活动\n");
} else if (vad_flag == WUKONG_AUDIO_VAD_END) {
    printf("语音活动结束\n");
}
```

## 实现细节

### 板级音频输入

- 使用 TKL 音频驱动进行硬件访问
- 实现环形缓冲区管理音频数据
- 支持手动和自动 VAD 模式
- 与 AEC/VAD 模块集成

### 板级音频输出

- 使用 TKL 音频驱动进行硬件访问
- 处理音频帧写入
- 支持音量控制

### UART 音频

- 提供基于 UART 的音频输入输出
- 适用于外部音频编解码芯片
- 目前支持GX8006: [获取唤醒词和固件](https://tuyaos.com/viewtopic.php?t=9147)
- 目前支持CI1302: [获取唤醒词和固件](https://tuyaos.com/viewtopic.php?t=9148)

## 事件系统

音频模块发布以下事件：

- `EVENT_AUDIO_VAD`: VAD 状态改变时发布
  - 数据: `WUKONG_AUDIO_VAD_FLAG_E` (START/END/NONE)

## 配置

### 编译时标志

- `USING_BOARD_AUDIO_INPUT`: 使用板级音频输入（默认）
- `USING_UART_AUDIO_INPUT`: 使用 UART 音频输入

### 运行时配置

- 采样率: 通常为 16kHz
- 采样位数: 16 位
- 声道: 单声道
- 帧大小: 320 个采样（16kHz 下为 20ms）
