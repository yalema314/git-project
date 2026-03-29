/**
 * @file wukong_kws.h
 * @brief 涂鸦唤醒词模块接口
 * 
 * 提供唤醒词相关功能的接口函数
 *
 * @copyright Copyright (c) 涂鸦科技
 *
 */

#ifndef __WUKONG_KWS_H__
#define __WUKONG_KWS_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! kws wakeup event name
#define EVENT_WUKONG_KWS_WAKEUP "event.kws.wakeup"

//! tuya defined keywords
typedef enum {
    WUKONG_KWS_NIHAOTUYA = 1,           //! 你好涂鸦
    WUKONG_KWS_XIAOZHITONGXUE = 2,      //! 小智同学
    WUKONG_KWS_HEYTUYA = 3,             //! hey-tuya
    WUKONG_KWS_SMARTLIFE = 4,           //! smartlife
    WUKONG_KWS_ZHINENGGUANJIA = 5,      //! 智能管家
    WUKONG_KWS_UDF1 = 6,                //! user defined keyword 1
    WUKONG_KWS_UDF2 = 7,                //! user defined keyword 2
    WUKONG_KWS_UDF3 = 8,                //! user defined keyword 3
    //! ... add as you want
} WUKONG_KWS_INDEX_E;

typedef struct {
    VOID    *handle;
    VOID    *priv_data;
} WUKONG_KWS_CTX_T;

typedef struct {
    INT_T  (*create)  (WUKONG_KWS_CTX_T *ctx);
    INT_T  (*detect)  (WUKONG_KWS_CTX_T *ctx, UINT8_T *data, UINT32_T datalen);
    INT_T  (*reset)   (WUKONG_KWS_CTX_T *ctx);
    INT_T  (*deinit)  (WUKONG_KWS_CTX_T *ctx);
    UINT8_T is_detect_vad;
} WUKONG_KWS_CFG_T;

INT_T wukong_kws_default_init(VOID); 

INT_T wukong_kws_init(WUKONG_KWS_CFG_T *cfg);
INT_T wukong_kws_uninit(VOID);
INT_T wukong_kws_feed_with_vad(UINT8_T *data, UINT16_T datalen, UINT8_T vadflag);

INT_T wukong_kws_enable(VOID);
INT_T wukong_kws_disable(VOID);
INT_T wukong_kws_set_vad_detect(UINT8_T is_detect_vad);
VOID wukong_kws_event(WUKONG_KWS_INDEX_E wakeup_kws_index);

#ifdef __cplusplus
}
#endif

#endif /* __WUKONG_KWS_H__ */
