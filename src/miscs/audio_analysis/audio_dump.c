/**
* Copyright (C) by Tuya Inc                                                  
* All rights reserved                                                        
*
* @file audio_dump.c
* @brief audio dump mic/ref/aec
* @version 1.0
* @author linch
* @date 2025-06-02
*
*/

#include "tuya_device_cfg.h"
#include "tuya_cloud_types.h"
#include "audio_dump.h"
#include "tal_memory.h"
#include "tal_uart.h"
#include "tal_log.h"
#include "tal_thread.h"
#include "audio_analysis.h"
#include "tkl_audio.h"
#if defined(ENABLE_APP_AI_MONITOR) && (ENABLE_APP_AI_MONITOR == 1)
#include "tuya_ai_monitor.h"
#endif

#define AUDIO_DUMP_BUF          1024*1024

typedef struct {
    uint8_t  *data;
    uint32_t  datalen;
} audio_dump_t;

STATIC audio_dump_t audio_dump[AUDIO_DUMP_MAX];
STATIC BOOL_T audio_dump_flag = FALSE;

VOID audio_dump_init(VOID);
VOID audio_dump_write(INT_T type, uint8_t *data, uint16_t datalen)
{
#if ENABLE_AUDIO_ANALYSIS
    STATIC INT_T init = 0;
    if (!init) {
        audio_dump_init();
        init = 1;
    }

    if (!audio_dump_flag) {
        return;
    }

    if (type >= AUDIO_DUMP_MAX) {
        return;
    }

    if (audio_dump[type].datalen + datalen > AUDIO_DUMP_BUF) {
        return;
    }
#if ENABLE_EXT_RAM
    memcpy(audio_dump[type].data + audio_dump[type].datalen, data, datalen);
    audio_dump[type].datalen += datalen;
#else
    if (__s_dump_type != AUDIO_DUMP_MAX) {
        audio_dump_with_uart(type, data, datalen);
    }
#endif
#endif // ENABLE_AUDIO_ANALYSIS
}

VOID audio_dump_enable(VOID)
{
    audio_dump_flag = true;
}

VOID audio_dump_disable(VOID)
{
    audio_dump_flag = false;
}

VOID audio_dump_reset(VOID)
{
    audio_dump[0].datalen = 0;
    audio_dump[1].datalen = 0;
    audio_dump[2].datalen = 0;

#ifndef ENABLE_EXT_RAM
    __s_dump_type = AUDIO_DUMP_MAX;
#endif    
}

VOID audio_dump_with_uart(INT_T type)
{
    TAL_PR_DEBUG("audio_dump type[%d] len %d\r\n", type, audio_dump[type].datalen);
#if ENABLE_EXT_RAM
    tal_uart_write(TUYA_UART_NUM_0, audio_dump[type].data , audio_dump[type].datalen);
    audio_dump[type].datalen = 0;
#else
    __s_dump_type = type;
#endif
}

VOID audio_dump_with_net(INT_T type)
{
    TAL_PR_DEBUG("audio_dump type[%d] len %d\r\n", type, audio_dump[type].datalen);

#if defined(ENABLE_APP_AI_MONITOR) && (ENABLE_APP_AI_MONITOR == 1)
    audio_dump_t *dump = &audio_dump[type];
    if (audio_dump && dump->data && dump->datalen > 0) {
        TAL_PR_DEBUG("net dump audio type %d, len %d", type, dump->datalen);
        // dump pcm data interval is 100ms
        for (int i = 0; i < dump->datalen; i += 3200) {
            TAL_PR_DEBUG("net dump audio data offset %d, len %d", i, MIN(3200, dump->datalen - i));
            AI_STREAM_TYPE stype = AI_STREAM_ONE;
            if (i == 0 && i + 3200 >= dump->datalen) {
                stype = AI_STREAM_ONE;
            } else if (i == 0) {
                stype = AI_STREAM_START;
            } else if (i + 3200 >= dump->datalen) {
                stype = AI_STREAM_END;
            } else {
                stype = AI_STREAM_ING;
            }

            if (type == 0) {
                tuya_ai_monitor_broadcast_audio_mic(stype, dump->data + i, MIN(3200, dump->datalen - i));
            } else if (type == 1) {
                tuya_ai_monitor_broadcast_audio_ref(stype, dump->data + i, MIN(3200, dump->datalen - i));
            } else if (type == 2) {
                tuya_ai_monitor_broadcast_audio_aec(stype, dump->data + i, MIN(3200, dump->datalen - i));
            }
        }
        dump->datalen = 0; // reset dump data length
    }
#else
    TAL_PR_ERR("ENABLE_APP_AI_MONITOR is not enabled, can not net dump audio");
#endif
}

VOID audio_play_bgm(INT_T type, INT_T freq)
{
    if (0 == type) {
        AUDIO_ANALYSIS_PARAMS_T params = {0};
        AUDIO_ANALYSIS_DEFAULT_PARAMS_GET_RANG(&params);
        audio_analysis_play(AUDIO_ANALYSIS_TYPE_RANG, &params);
    } else if (1 == type) { /*单频*/
        AUDIO_ANALYSIS_PARAMS_T params = {0};
        AUDIO_ANALYSIS_DEFAULT_PARAMS_GET_SINGLE(&params);
        if (freq > 0) {
            params.freq = freq;
        }                
        audio_analysis_play(AUDIO_ANALYSIS_TYPE_SINGLE, &params);
    } else if (2 == type) { /*白噪声*/
        AUDIO_ANALYSIS_PARAMS_T params = {0};
        AUDIO_ANALYSIS_DEFAULT_PARAMS_GET_SWEEP(&params);
        audio_analysis_play(AUDIO_ANALYSIS_TYPE_SWEEP, &params);
    } else if (3 == type) { /*调频*/                
        AUDIO_ANALYSIS_PARAMS_T params = {0};
        INT_T amp = freq;
        if (amp < 0) {
            params.amp = amp;
        }
        AUDIO_ANALYSIS_DEFAULT_PARAMS_GET_SWEEPSPECIAL(&params);
        audio_analysis_play(AUDIO_ANALYSIS_TYPE_SWEEPSPECIAL, &params);
    } else if (4 == type) { /*最小信号*/
        AUDIO_ANALYSIS_PARAMS_T params = {0};
        AUDIO_ANALYSIS_DEFAULT_PARAMS_GET_MINSIN(&params);
        if (freq > 0) {
            params.freq = freq;
        }                
        audio_analysis_play(AUDIO_ANALYSIS_TYPE_MINSIN, &params);
    } else {
        TAL_PR_ERR("unknown audio test data %d", type);
    }
}

VOID audio_set_volume(INT_T volume)
{
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, TKL_AO_0, NULL, volume);
}

VOID audio_set_micgain(INT_T micgain)
{
    if (micgain < 0) {
        micgain = 0;
    } else if (micgain > 100) {
        micgain = 100;
    }

    tkl_ai_set_vol(TKL_AUDIO_TYPE_BOARD, 0, micgain);
}

// VOID audio_ctrl_alg(INT_T argc, CHAR_T *argv[])
// {
//     if (0 == strcmp(argv[2], "set")) {
//         // ! ao alg set <para> <value>
//         if (argc != 5) {
//             TAL_PR_DEBUG("audio alg set cmd error\r\n");
//             return;
//         }
//         INT_T i;
//         for (i = 0; i < sizeof(audio_alg_para_map) / sizeof(aa_alg_para_map_t); i++) {
//             if (0 == strcmp(argv[3], audio_alg_para_map[i].name)) {
//                 break;
//             }
//         }
//         if (i >= sizeof(audio_alg_para_map) / sizeof(aa_alg_para_map_t)) {
//             TAL_PR_DEBUG("audio alg set para %s not found\r\n", argv[3]);
//             return;
//         }
//         uint32_t type = audio_alg_para_map[i].para;
//         uint32_t value = atoi(argv[4]);
//         _audio_test_event(AUDIO_TEST_EVENT_SET_ALG_PARA, type, value);
//     } else if (0 == strcmp(argv[2], "get")) {
//         // ! ao alg get <para>
//         if (argc != 4 && argc != 5) {
//             return;
//         }
//         INT_T i;
//         for (i = 0; i < sizeof(audio_alg_para_map) / sizeof(aa_alg_para_map_t); i++) {
//             if (0 == strcmp(argv[3], audio_alg_para_map[i].name)) {
//                 break;
//             }
//         }
//         if (i >= sizeof(audio_alg_para_map) / sizeof(aa_alg_para_map_t)) {
//             TAL_PR_DEBUG("audio alg get para %s not found\r\n", argv[3]);
//             return;
//         }
//         uint32_t type = audio_alg_para_map[i].para;
//         uint32_t value = 0;
//         if (argc == 5) {
//             value = atoi(argv[4]);
//         }
//         _audio_test_event(AUDIO_TEST_EVENT_GET_ALG_PARA, type, value);
//     } else if (0 == strcmp(argv[2], "dump")) {
//         // ! ao alg dump
//         _audio_test_event(AUDIO_TEST_EVENT_DUMP_ALG_PARA, 0, 0);
//     } else {
//         TAL_PR_DEBUG("audio alg cmd error\r\n");
//     }
// }

//！ ao start
//！ ao stop
//！ ao reset
//！ ao dump 0
//！ ao dump 1
//！ ao dump 2
//！ ao netdump 0
//！ ao netdump 1
//！ ao netdump 2
//！ ao bg 0
//！ ao bg 1 (ao bg 1 1000)
//！ ao bg 2
//！ ao volume 50
// ! ao micgain 70(default)
// ! ao alg set <para> [<para2>] <value>
// ! ao alg get <para> [<para2>]
// ! ao alg dump
// ! ao echo <info>
VOID audio_dump_exec(INT_T argc, CHAR_T *argv[])
{
    if (0 == strcmp(argv[1], "start")) {
        audio_dump_enable();
        TAL_PR_DEBUG("audio_dump start\r\n");
    } else if (0 == strcmp(argv[1], "stop")) {
        audio_dump_disable();
        TAL_PR_DEBUG("audio_dump stop\r\n");
    } else if (0 == strcmp(argv[1], "dump")) {
        audio_dump_with_uart(atoi(argv[2]));
        TAL_PR_DEBUG("audio_dump, %d\r\n", atoi(argv[2]));
    } else if (0 == strcmp(argv[1], "netdump")) {
        audio_dump_with_net(atoi(argv[2]));
        TAL_PR_DEBUG("audio_dump, %d\r\n", atoi(argv[2]));
    } else if (0 == strcmp(argv[1], "reset")) {
        audio_dump_reset();
        TAL_PR_DEBUG("audio_dump reset\r\n");
    } else if (0 == strcmp(argv[1], "bg")) {
        INT_T freq = 0;
        if (argc > 3) {
            freq = atoi(argv[3]);
        }
        audio_play_bgm(atoi(argv[2]), freq);
        TAL_PR_DEBUG("audio_dump play bgm %d\r\n", atoi(argv[2]));
    } else if (0 == strcmp(argv[1], "volume")) {
        audio_set_volume(atoi(argv[2]));
        TAL_PR_DEBUG("audio_dump set volume %d\r\n", atoi(argv[2]));
    } else if (0 == strcmp(argv[1], "micgain")) {
        audio_set_micgain(atoi(argv[2]));
        TAL_PR_DEBUG("audio_dump set micgain %d\r\n", atoi(argv[2]));
    } else if (0 == strcmp(argv[1], "alg")) {
        // audio_ctrl_alg(argc, argv);
    } else if (0 == strcmp(argv[1], "echo")) {
        TAL_PR_DEBUG("echo %s", argv[2]);
    } else {
        TAL_PR_DEBUG("audio_dump cmd error\r\n");
    }
}


INT_T strsplit(CHAR_T* input, INT_T *argc, CHAR_T *argv[])
{
    CONST CHAR_T delimiter[] = " ";
    CHAR_T *token = NULL;

    *argc = 0;
    // Get the first token
    token = strtok(input, delimiter);
    argv[(*argc)++] = token;
    TAL_PR_DEBUG("token %s\r\n", token);
    // Iterate over tokens
    while (token != NULL) {
        // Get the next token
        token = strtok(NULL, delimiter);
        if (token) {
            argv[(*argc)++] = token;
        }
        if (*argc >= 10) {
            return OPRT_INDEX_OUT_OF_BOUND;
        }
    }

    return OPRT_OK;
}

VOID __audio_dump_task(VOID *params)
{
    INT_T     rt;

    TAL_UART_CFG_T cfg = {0};
    cfg.base_cfg.baudrate = 460800;
    cfg.base_cfg.databits = TUYA_UART_DATA_LEN_8BIT;
    cfg.base_cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
    cfg.base_cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
    cfg.rx_buffer_size = 256;
    cfg.open_mode = O_BLOCK;
    rt = tal_uart_init(TUYA_UART_NUM_0, &cfg);

    uint8_t ch;
    uint8_t buffer[255];
    uint8_t index = 0;
    INT_T     argc;
    CHAR_T   *argv[10];

    for (;;) {
        tal_uart_read(TUYA_UART_NUM_0, (UINT8_T*)&ch, 1);
        if (ch != '\r' && ch != '\n') {
            buffer[index++] = ch;
            continue;
        }
        buffer[index] = '\0';
        //! if '\r\n' is end of CHAR_T, '\n' is need discard， so check index
        if (index && OPRT_OK == strsplit(buffer, &argc, argv)) {
            TAL_PR_DEBUG("dump command:%s %s", argv[0], argv[1]);
            //! parse cmd
            if (0 == strcmp(argv[0], "ao")) {
                audio_dump_exec(argc, argv);
            }
        }
        index = 0;
    }
}

#include "tkl_thread.h"

STATIC TKL_THREAD_HANDLE audio_dump_handle = NULL;

VOID audio_dump_init(VOID)
{
    INT_T i = 0;
    for (i = 0; i < AUDIO_DUMP_MAX; i++) {
        audio_dump[i].datalen = 0;
#if ENABLE_EXT_RAM        
        audio_dump[i].data    = tal_psram_malloc(AUDIO_DUMP_BUF);
#else
        audio_dump[i].data    = NULL;        
#endif        
    }

    THREAD_CFG_T thread_param = {0};
    thread_param.stackDepth = 1024*4;
    thread_param.priority = THREAD_PRIO_1;
    thread_param.thrdname = "audio_dump";
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    thread_param.psram_mode = 1;
#endif
    tal_thread_create_and_start(&audio_dump_handle, NULL, NULL, __audio_dump_task, NULL, &thread_param);
}
