<p align="center"><a href="https://tuyaos.com" target="_blank"><img src="https://github.com/tuya/.github/raw/main/profile/site_logo.png" width="400"></a></p>

## 概述

此示例是开源开放的，帮助大家理解如何使用 `Wukong AI` 硬件开发框架开发产品，希望起到抛砖引玉的作用。示例工程没有经过完整的测试流程，难免有一些问题，细节也没经过打磨，如果大家在使用的时候遇到问题，可以自行修改代码，也可以到 `TuyaOS` 开发者论坛 [联网单品开发版块](https://www.tuyaos.com/viewforum.php?f=11) 发帖咨询。如果有开发者直接基于此示例进行量产，需要自行对产品的功能进行完整的测试和验证，涂鸦不对此示例产生的不利后果负责。

## 项目简介

`tuyaos_demo_wukong_ai` 是一个基于涂鸦 TuyaOS 框架的 AI 智能硬件开发示例项目，展示了如何使用 Wukong AI 硬件开发框架构建智能语音交互设备。该项目支持多种交互模式、多模态输入输出（音频、视频、图像），并提供了完整的 AI 对话能力。

## 主要功能特性

### 核心功能
- **多种模式 AI 对话**：支持长按触发、单次按键、关键词唤醒、自由对话、P2P、翻译等多种交互模式
- **多模态 AI 支持**：支持音频输入输出、摄像头视频输入、图像识别与生成
- **端侧 MCP 能力**：支持音量调节、拍照识物等端侧 Model Context Protocol 功能
- **AI 技能系统**：内置时钟、情绪识别、音乐故事、云端事件等多种技能
- **唤醒词识别**：集成通用微、志骏等唤醒词库，支持自定义唤醒词
- **音频处理**：支持 AEC（回声消除）、VAD（语音活动检测）、音频编解码，支持音频开发过程中的测试、分析、调试。

### 硬件支持
请参考[开发板支持情况](./src/boards/README_CN.md)

### UI 框架
- **LVGL 支持**：支持 LVGL v8/v9 图形库
- **GUI 组件**：提供天气、资源下载、DP2Widget、云端配方等 GUI 组件
- **字体渲染**：集成 FreeType 字体渲染引擎

## 项目目录结构

```
tuyaos_demo_wukong_ai/
├── src/                          # 源代码目录
│   ├── wukong/                   # Wukong AI 核心模块
│   ├── drivers/                  # 硬件驱动
│   ├── miscs/                    # 功能模块
│   ├── boards/                   # 板级支持
│   └── tuya_ai_toy.c             # 主应用逻辑
├── include/                      # 头文件目录
├── docs/                         # 文档目录
├── scripts/                      # 脚本工具
└── build/                        # 编译输出目录
```

### 子目录说明

#### 📁 src/wukong/
Wukong AI 核心模块，包含 AI 对话、音频处理、模式管理、技能系统等核心功能。

**详细文档**: [src/wukong/README_CN.md](src/wukong/README_CN.md)

主要子模块：
- `audio/` - 音频输入输出、AEC/VAD、播放器
- `kws/` - 关键词唤醒检测
- `mode/` - AI 对话模式管理
- `skills/` - AI 技能系统
- `mcp/` - Model Context Protocol 服务器
- `video/` - 视频输入输出处理
- `picture/` - 图像输入输出处理

#### 📁 src/boards/
板级支持包（BSP），包含不同硬件开发板的配置和 UI 实现。

**详细文档**: [src/boards/README_CN.md](src/boards/README_CN.md)

支持的开发板：
- `T5AI_BOARD/` - 标准开发板 + 3.5寸 RGB 屏开发板
- `T5AI_BOARD_EYES/` - 标准开发板 + 眼睛开发板
- `T5AI_BOARD_EVB/` - AI语音盒子
- `T5AI_BOARD_ROBOT/` - I机器人界面
- `T5AI_BOARD_DESKTOP/` - 桌面AI助手应用
- `Ubuntu/` - Ubuntu 模拟环境

#### 📁 src/drivers/
硬件驱动层，提供摄像头、显示、按键、LED、触摸屏、IMU 等硬件驱动的抽象接口。

**详细文档**: [src/drivers/README.md](src/drivers/README.md)

主要驱动模块：
- `app_tuya_camera/` - 摄像头驱动（DVP/UVC）
- `app_tuya_display/` - 显示驱动（SPI/8080/RGB/QSPI）
- `app_tuya_key/` - 按键驱动
- `app_tuya_led/` - LED 驱动
- `app_tuya_tp/` - 触摸屏驱动
- `app_tuya_imu/` - IMU 传感器驱动

#### 📁 src/miscs/
功能模块层，包含音频播放、GUI、P2P 通信、电池管理等功能模块。

**详细文档**: [src/miscs/README.md](src/miscs/README.md)

主要功能模块：
- `audio_player/` - 音频播放器
- `audio_analysis/` - 音频分析工具
- `gui/` - GUI 相关库（LVGL、FreeType 等）
- `p2p/` - P2P 点对点通信
- `battery/` - 电池管理
- `uart_codec/` - UART 编解码 (涂鸦串口语音协议)
- `servo_ctrl/` - 舵机控制

#### 📁 docs/
项目文档目录，包含开发指南、调试文档、硬件资料等。

主要文档：
- `T5音频调试指南.md` - 音频调试指南

#### 📁 scripts/
脚本工具目录，提供音频测试、调试、资源生成等工具。

主要脚本：
- `audio_test.py` - 音频测试工具
- `audio_uart_dump.py` - UART 音频数据导出
- `ai_audio_proc.py` - AI 音频处理工具
- `gen_media_src.py` - 媒体资源生成工具

#### 📁 include/
头文件目录，包含项目的主要头文件定义。

#### 📁 build/
编译输出目录，包含编译生成的配置文件和固件。

## 快速开始

### 配置工程

```shell
make app_menuconfig APP_NAME=tuyaos_demo_wukong_ai 
```

* 示例同时支持音频+摄像头+屏幕功能，其中默认支持语音 + `UI`，摄像头需要配置打开；也可以配置关闭 `UI`：

![](https://images.tuyacn.com/fe-static/docs/img/d438d90d-88bd-4191-89f9-8e828fe9204d.png)


### 生成配置文件

```c
make app_config APP_NAME=tuyaos_demo_wukong_ai
```

配置完成之后（选择 [`T5AI_BOARD`](https://developer.tuya.com/cn/docs/iot-device-dev/T5-E1-IPEX-development-board?id=Ke9xehig1cabj)），必须运行上述命令生成新的 `tuya_app_config.h` 文件，否则配置不会生效。


## 软件说明

示例程序的产品创建、编译、烧录、调试过程请参考[`Wukong` `AI` 硬件开发框架-快速开始](https://developer.tuya.com/cn/docs/iot-device-dev/quick-start?id=Kectxdshpvsqr)。

### 代码架构
项目采用模块化设计，主要包含以下层次：
1. **应用层** (`src/tuya_ai_toy.c`)：主应用逻辑，协调各个模块
2. **AI 核心层** (`src/wukong/`)：AI 对话、模式管理、技能系统
3. **驱动层** (`src/drivers/`)：硬件抽象层，提供统一的硬件接口
4. **功能模块层** (`src/miscs/`)：音频播放、GUI、P2P 等功能模块
5. **板级支持层** (`src/boards/`)：不同开发板的特定配置

### 本地提示音
示例程序目前仅内置了一个“叮咚”的提示音，用于在无网络情况下的提示，其他状态均使用网络提示音。网络提示音可以更好的支持多语言，保持提示音的音色和角色一致，但也有缺点，会有延迟和干扰。

如果需要修改提示音，替换自行替换，具体操作参考:[本地提示语音播放能力](https://developer.tuya.com/cn/docs/developer/local-voice-prompt?id=Kehkq7n11rgas)。

如果不想使用云端提示音，请关闭`./include/tuya_device_cfg.h`中的配置，并修改相关代码：
```c
// 关闭cloud alert
#define ENABLE_CLOUD_ALERT 0

// 修改此函数，按需使用对应的本地提示音
OPERATE_RET wukong_audio_player_alert(TY_AI_TOY_ALERT_TYPE_E type, BOOL_T send_eof)
{
    TAL_PR_NOTICE("wukong audio player -> play alert type=%d", type);

    OPERATE_RET rt = OPRT_OK;
    CONST CHAR_T *audio_data = NULL;
    UINT32_T audio_size = 0;

    switch (type) {
    case AI_TOY_ALERT_TYPE_POWER_ON:
    case AI_TOY_ALERT_TYPE_NETWORK_CONNECTED:
    case AI_TOY_ALERT_TYPE_BATTERY_LOW:
    case AI_TOY_ALERT_TYPE_PLEASE_AGAIN:
    case AI_TOY_ALERT_TYPE_LONG_KEY_TALK:
    case AI_TOY_ALERT_TYPE_KEY_TALK:
    case AI_TOY_ALERT_TYPE_WAKEUP_TALK:
    case AI_TOY_ALERT_TYPE_RANDOM_TALK:
    case AI_TOY_ALERT_TYPE_WAKEUP:  
#if defined(ENABLE_CLOUD_ALERT) && ENABLE_CLOUD_ALERT==1    
        if (OPRT_OK == wukong_ai_agent_cloud_alert(type)) break;
#endif
    case AI_TOY_ALERT_TYPE_NOT_ACTIVE:
    case AI_TOY_ALERT_TYPE_NETWORK_CFG:
    case AI_TOY_ALERT_TYPE_NETWORK_FAIL:
    case AI_TOY_ALERT_TYPE_NETWORK_DISCONNECT:      
    default:
        audio_data = (CONST CHAR_T*)media_src_dingdong_zh;
        audio_size = sizeof(media_src_dingdong_zh);   
        break;
    }

    TUYA_CALL_ERR_LOG(wukong_audio_play_data(AI_AUDIO_CODEC_MP3, audio_data, audio_size));
    return rt;
}
```

### 对话模式说明
目前内置了多种对话模式，每种模式便于修改，分别独立，代码简单，结构清楚：

- **HOLD 模式** (`wukong_ai_mode_hold.c`)：长按触发模式，按住按键时进行对话
- **ONESHOT 模式** (`wukong_ai_mode_oneshot.c`)：单次按键触发，回合制对话模式
- **WAKEUP 模式** (`wukong_ai_mode_wakeup.c`)：关键词唤醒模式，唤醒后单轮对话
- **FREE 模式** (`wukong_ai_mode_free.c`)：关键词唤醒和自由对话模式，支持连续对话
- **P2P 模式** (`wukong_ai_mode_p2p.c`)：点对点通信模式
- **TRANSLATE 模式** (`wukong_ai_mode_translate.c`)：翻译模式

如果需要定制自己的模式，可以参考现有模式的实现，在 `src/wukong/mode/` 目录下添加新的模式文件，并在 `wukong_ai_mode.c` 中注册。

### 替换唤醒库
示例程序目前开放了通用微、志骏提供的默认唤醒词库，以及相关处理的流程。如果有替换唤醒词的需求，参考`src/wukong/kws/wukong_kws.c`中的实现来适配。

### 端侧MCP能力
示例程序目前仅内置调节音量、拍照识物的端侧MCP能力，可以参考`src/tuya_ai_toy_mcp.c`中的示例实现其他的功能。

## 音频调试
硬件、声学结构的测试继续参考之前的调试工具和方法：[T5音频调试指南](./docs/T5音频调试指南.md)。

`VAD`算法灵敏度调节：
```c
typedef enum {
    WUKONG_AUDIO_VAD_HIGH, // 灵敏度低
    WUKONG_AUDIO_VAD_MID,  // 灵敏度中
    WUKONG_AUDIO_VAD_LOW,  // 灵敏度高
} WUKONG_AUDIO_VAD_THRESHOLD_E;

// 默认使用中等级别
wukong_vad_set_threshold(WUKONG_AUDIO_VAD_MID);
```

`VAD`算法响应时间调节：
```c
STATIC OPERATE_RET __ai_toy_wukong_ai_agent_init()
{
    ...                                            
    audio_cfg.board.vad_off_ms     = 1000;  // 1000ms timeout                                          
    audio_cfg.board.vad_active_ms  = 200;   // 200ms active                                             
    ...

    return rt;
}
```

### 编译固件

完成工程配置、生成文件之后，即可编译工程，通过`Tuya Wind IDE` 右键菜单一键编译工程，即可生成对应的固件。


## 技术支持

如果开发过程遇到问题，可以：
1. 查看各子目录下的 README 文档获取详细说明
2. 到 TuyaOS 开发者论坛 [联网单品开发版块](https://www.tuyaos.com/viewforum.php?f=11) 发帖咨询
3. 参考涂鸦开发者文档：[Wukong AI 硬件开发框架](https://developer.tuya.com/cn/docs/iot-device-dev/quick-start?id=Kectxdshpvsqr)




