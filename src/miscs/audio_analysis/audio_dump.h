/**
 * @file audio_dump.h
 * @brief audio_dump module is used to 
 * @version 0.1
 * @date 2025-06-25
 */

#ifndef __AUDIO_DUMP_H__
#define __AUDIO_DUMP_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/
typedef uint32_t AUDIO_TEST_EVENT_E;
#define AUDIO_TEST_EVENT_PLAY_BGM          0x01
#define AUDIO_TEST_EVENT_SET_VOLUME        0x02
#define AUDIO_TEST_EVENT_SET_MICGAIN       0x03
#define AUDIO_TEST_EVENT_SET_ALG_PARA      0x04
#define AUDIO_TEST_EVENT_GET_ALG_PARA      0x05
#define AUDIO_TEST_EVENT_DUMP_ALG_PARA     0x06
#define AUDIO_TEST_EVENT_NET_DUMP_AUDIO    0x07

#define AUDIO_DUMP_MIC          0
#define AUDIO_DUMP_REF          1
#define AUDIO_DUMP_AEC          2
#define AUDIO_DUMP_KWS          3
#define AUDIO_DUMP_VAD          4
#define AUDIO_DUMP_MAX          5
/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/

VOID audio_dump_write(INT_T type, uint8_t *data, uint16_t datalen);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_DUMP_H__ */
