#include "tal_memory.h"
#include "tal_system.h"
#include "tal_log.h"
#include "tuya_app_config.h"
#include "audio_dump.h"
#include "wukong_audio_aec_vad.h"
#include "wukong_kws.h"
#include "speexdsp/audio_subsys_speexdsp_wrap2.h"
#include "speexdsp/audio_subsys_rnn_vad.h"

#define TUYA_AEC_VAD_DEBUG 0

STATIC VOID *__s_speex_aec_handle = NULL;
STATIC VOID *__s_rnn_vad_handle = NULL;
STATIC UINT16_T *__s_linearaec = NULL;
STATIC UINT32_T __s_frame_size = 0;
STATIC WUKONG_AUDIO_VAD_FLAG_E __s_aec_vad_flag = WUKONG_AUDIO_VAD_STOP;

OPERATE_RET wukong_aec_vad_init(UINT32_T min_speech_len_ms, UINT32_T max_speech_interval_ms, UINT32_T frame_size)
{
    OPERATE_RET rt = OPRT_OK;
#if !defined(WUKONG_BOARD_UBUNTU) || (WUKONG_BOARD_UBUNTU == 0)
    if (__s_speex_aec_handle==NULL) {
		__s_speex_aec_handle = speex_aes_create(frame_size/2);
        TUYA_CHECK_NULL_RETURN(__s_speex_aec_handle, OPRT_COM_ERROR);
	}
    
    if (__s_rnn_vad_handle == NULL) {
        __s_rnn_vad_handle = rnn_vad_create();
        struct _rnn_vad_param_in param = {0};
        param.min_speech_len = min_speech_len_ms;         //最短语音长度(ms)，太小会导致误报,默认200ms
        param.max_speech_interval = max_speech_interval_ms;   //最大语音间隔(ms) 太小会导致断句过快，默认1000ms
        rnn_vad_init(&param, __s_rnn_vad_handle);
        wukong_vad_set_threshold(WUKONG_AUDIO_VAD_LOW);
        TUYA_CHECK_NULL_GOTO(__s_rnn_vad_handle, __err_exit);
    }

    if (__s_linearaec == NULL) {
#ifdef ENABLE_EXT_RAM        
        __s_linearaec = tal_psram_malloc(frame_size * 2);
        TUYA_CHECK_NULL_GOTO(__s_linearaec, __err_exit);
#else   
        __s_linearaec = tal_malloc(frame_size * 2);
        TUYA_CHECK_NULL_RETURN(__s_linearaec, OPRT_MALLOC_FAILED);
#endif
    }
    __s_frame_size = frame_size;
    tkl_ai_set_vad_aec_algorithm(wukong_aec_vad_process);
    TAL_PR_DEBUG("wukong aec -> init, frame size %d, aec %p, vad %p, lineraraec %p", __s_frame_size, __s_speex_aec_handle, __s_rnn_vad_handle, __s_linearaec);
    return OPRT_OK;
__err_exit:
    tkl_ai_set_vad_aec_algorithm(NULL);
    wukong_aec_vad_deinit();
#endif
    return rt;
}

OPERATE_RET wukong_aec_vad_deinit(VOID)
{
#if !defined(WUKONG_BOARD_UBUNTU) || (WUKONG_BOARD_UBUNTU == 0)
    if (__s_linearaec) {
        tal_free(__s_linearaec);
        __s_linearaec = NULL;
    }

    if (__s_rnn_vad_handle) {
        rnn_vad_destroy(__s_rnn_vad_handle);
        __s_rnn_vad_handle = NULL;
    }

    if (__s_speex_aec_handle) {
        speex_aes_destory(__s_speex_aec_handle);
        __s_speex_aec_handle = NULL;
    }

    __s_frame_size = 0;
#endif
    return OPRT_OK;
}


OPERATE_RET wukong_vad_set_threshold(WUKONG_AUDIO_VAD_THRESHOLD_E level)
{
#if !defined(WUKONG_BOARD_UBUNTU) || (WUKONG_BOARD_UBUNTU == 0)
    if (__s_rnn_vad_handle) {
        switch (level)
        {
        case WUKONG_AUDIO_VAD_HIGH:
            rnn_vad_set_callback(__s_rnn_vad_handle, -30);
            break;
        case WUKONG_AUDIO_VAD_MID:
            rnn_vad_set_callback(__s_rnn_vad_handle, -40);
            break;
        case WUKONG_AUDIO_VAD_LOW:            
            rnn_vad_set_callback(__s_rnn_vad_handle, -70);
            break;
        default:
            break;
        }

        return OPRT_OK;
    }

    return OPRT_RESOURCE_NOT_READY;
#else
    return OPRT_OK;
#endif
}

OPERATE_RET wukong_vad_start(VOID)
{
#if !defined(WUKONG_BOARD_UBUNTU) || (WUKONG_BOARD_UBUNTU == 0)
    __s_aec_vad_flag = WUKONG_AUDIO_VAD_STOP;
    if (__s_rnn_vad_handle) {
        rnn_vad_start(__s_rnn_vad_handle);
    }
#endif
    return OPRT_OK;
}

OPERATE_RET wukong_vad_stop(VOID)
{
#if !defined(WUKONG_BOARD_UBUNTU) || (WUKONG_BOARD_UBUNTU == 0)
    __s_aec_vad_flag = WUKONG_AUDIO_VAD_STOP;
    if (__s_rnn_vad_handle) {
        rnn_vad_stop(__s_rnn_vad_handle);
    }
#endif
    return OPRT_OK;
}

OPERATE_RET wukong_aec_vad_process(INT16_T *mic_data, INT16_T *ref_data, INT16_T *out_data)
{
    TUYA_CHECK_NULL_RETURN(mic_data, OPRT_INVALID_PARM);
    TUYA_CHECK_NULL_RETURN(ref_data, OPRT_INVALID_PARM);
    TUYA_CHECK_NULL_RETURN(out_data, OPRT_INVALID_PARM);
#if !defined(WUKONG_BOARD_UBUNTU) || (WUKONG_BOARD_UBUNTU == 0)
 	if (__s_speex_aec_handle) {
		UINT32_T begin = tal_system_get_millisecond();
		speex_aes_process(__s_speex_aec_handle, (short*)mic_data, (short*)ref_data, (short*)out_data);

        audio_dump_write(AUDIO_DUMP_MIC, mic_data, __s_frame_size);
        audio_dump_write(AUDIO_DUMP_REF, ref_data, __s_frame_size);
        audio_dump_write(AUDIO_DUMP_AEC, out_data, __s_frame_size);

		UINT32_T end = tal_system_get_millisecond();
#if defined(TUYA_AEC_VAD_DEBUG) && (TUYA_AEC_VAD_DEBUG == 1)
		STATIC INT_T cnt = 0;
		if (cnt++ % 500 == 0) {
			TAL_PR_DEBUG("speexdsp process time: aec->%d(ms), count = %d\r\n", end - begin, cnt);
		}
#endif
	}
	if (__s_speex_aec_handle && __s_rnn_vad_handle && __s_linearaec) {
		UINT32_T begin = tal_system_get_millisecond();
		BOOL_T has_vad = rnn_vad_process(__s_rnn_vad_handle, (short *)out_data);
		if (has_vad && __s_aec_vad_flag != WUKONG_AUDIO_VAD_START) {
			TAL_PR_DEBUG("################ [vad start] ################ \r\n");
            __s_aec_vad_flag = WUKONG_AUDIO_VAD_START;
		} else if (!has_vad && __s_aec_vad_flag != WUKONG_AUDIO_VAD_STOP) {
			TAL_PR_DEBUG("################ [vad stop] ################ \r\n");
            __s_aec_vad_flag = WUKONG_AUDIO_VAD_STOP;
		} 
		UINT32_T end = tal_system_get_millisecond();

#if defined(TUYA_AEC_VAD_DEBUG) && (TUYA_AEC_VAD_DEBUG == 1)
		STATIC INT_T cnt = 0;
		if (cnt++ % 500 == 0) {
			TAL_PR_DEBUG("rnn_date_version: 25082101 \r\n");
			TAL_PR_DEBUG("rnn vad process time: rnn vad->%d(ms), flag=%d, count = %d\r\n", end - begin, __s_aec_vad_flag, cnt);
		}
#endif

        speex_get_param(__s_speex_aec_handle, NULL, (short*)__s_linearaec);
        wukong_kws_feed_with_vad(__s_linearaec, __s_frame_size, __s_aec_vad_flag==WUKONG_AUDIO_VAD_START?1:0);
	}
#endif
    return OPRT_OK;
}

INT_T wukong_vad_get_flag(VOID)
{
    return __s_aec_vad_flag;
}
