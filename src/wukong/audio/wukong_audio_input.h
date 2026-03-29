/**
* Copyright (C) by Tuya Inc                                                  
* All rights reserved                                                        
*
* @file wukong_audio_input.h
* @brief audio input and output processing
* @version 1.0
* @author linch
* @date 2025-05-13
*
*/

#ifndef __WUKONG_AUDIO_INPUT_H__
#define __WUKONG_AUDIO_INPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "tkl_audio.h"
#include "tdl_comm_audio.h"

#define EVENT_AUDIO_VAD   "EVENT.VAD"

typedef enum {
    WUKONG_AUDIO_VAD_MANIAL,    // use key event as vad
    WUKONG_AUDIO_VAD_AUTO,      // use human voice detect 
} WUKONG_AUDIO_VAD_MODE_E;

typedef enum {
    WUKONG_AUDIO_USING_BOARD,    // use board audio
    WUKONG_AUDIO_USING_UART,     // use uart audio
} WUKONG_AUDIO_TYPE_E;

// typedef struct {
//     UINT8_T                 *data;
//     UINT16_T                datalen;
// } WUKONG_AUDIO_MSG;
typedef INT_T (*WUKONG_AUDIO_OUTPUT)(UINT8_T *data, UINT16_T datalen);

typedef struct {
    //! mic hardware (speaker used same configure)
    TKL_AUDIO_SAMPLE_E          sample_rate;       // audio sample rate
    TKL_AUDIO_DATABITS_E        sample_bits;       // audio sample bits
    TKL_AUDIO_CHANNEL_E         channel;           // audio channel
    TUYA_GPIO_NUM_E             spk_io;            // speaker enable pin
    TUYA_GPIO_LEVEL_E           spk_io_level;      // pin polarity, 0 high enable, 1 low enable

    //! vad cache = vad_active_ms + vad_off_ms
    WUKONG_AUDIO_VAD_MODE_E     vad_mode;
    UINT16_T                    vad_off_ms;        // 语音活动补偿时间，单位ms
    UINT16_T                    vad_active_ms;     // 语音活动检测阈值，单位ms
    UINT16_T                    slice_ms;          // ref macro, AUDIO_RECORDER_SLICE_TIME

    //! on mic data process
    WUKONG_AUDIO_OUTPUT         output_cb;
    VOID                        *user_data;
} WUKONG_BOARD_AUDIO_INPUT_CFG_T;

typedef struct  {
    WUKONG_AUDIO_TYPE_E type;
    UINT16_T timeout;
    union {
        WUKONG_BOARD_AUDIO_INPUT_CFG_T board;
        TDL_COMM_AUDIO_CFG_T uart;
    }; 
} WUKONG_AUDIO_INPUT_CFG_T;


typedef struct {
    OPERATE_RET (*init)(WUKONG_AUDIO_INPUT_CFG_T *cfg);
    OPERATE_RET (*deinit)(VOID);
    OPERATE_RET (*start)(VOID);
    OPERATE_RET (*stop)(VOID);
    OPERATE_RET (*reset)(VOID);
    OPERATE_RET (*set_wakeup)(BOOL_T is_wakeup);
    OPERATE_RET (*set_vad_mode)(WUKONG_AUDIO_VAD_MODE_E mode);
} WUKONG_AUDIO_INPUT_PRODUCER_T;
extern WUKONG_AUDIO_INPUT_PRODUCER_T g_audio_input_producer;

OPERATE_RET wukong_audio_input_init(WUKONG_AUDIO_INPUT_CFG_T *cfg);
OPERATE_RET wukong_audio_input_start(VOID);
OPERATE_RET wukong_audio_input_stop(VOID);
OPERATE_RET wukong_audio_input_wakeup_mode_set(WUKONG_AUDIO_VAD_MODE_E mode);
OPERATE_RET wukong_audio_input_wakeup_set(BOOL_T is_wakeup);
OPERATE_RET wukong_audio_input_reset(VOID);
OPERATE_RET wukong_audio_input_deinit(VOID);

#ifdef __cplusplus
}
#endif

#endif /* __TUYA_AUDIO_RECORDER_H__ */
