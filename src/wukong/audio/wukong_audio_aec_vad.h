/**
 * @file wukong_kws.h
 * @brief 涂鸦唤醒词模块接口
 * 
 * 提供唤醒词相关功能的接口函数
 *
 * @copyright Copyright (c) 涂鸦科技
 *
 */

#ifndef __WUKONG_AUDIO_AEC_VAD_H__
#define __WUKONG_AUDIO_AEC_VAD_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WUKONG_AUDIO_VAD_START = 1,
    WUKONG_AUDIO_VAD_STOP,
} WUKONG_AUDIO_VAD_FLAG_E;

typedef enum {
    WUKONG_AUDIO_VAD_HIGH,
    WUKONG_AUDIO_VAD_MID,
    WUKONG_AUDIO_VAD_LOW,
} WUKONG_AUDIO_VAD_THRESHOLD_E;


OPERATE_RET wukong_aec_vad_init(UINT32_T min_speech_len_ms, UINT32_T max_speech_interval_ms, UINT32_T frame_size); 
OPERATE_RET wukong_aec_vad_deinit(VOID); 
OPERATE_RET wukong_aec_vad_process(INT16_T *mic_data, INT16_T *ref_data, INT16_T *out_data); 
OPERATE_RET wukong_vad_set_threshold(WUKONG_AUDIO_VAD_THRESHOLD_E level); 
OPERATE_RET wukong_vad_start(VOID); 
OPERATE_RET wukong_vad_stop(VOID); 
INT_T wukong_vad_get_flag(VOID); 


#ifdef __cplusplus
}
#endif

#endif /* __WUKONG_AUDIO_AEC_VAD_H__ */
