#ifndef __WUKONG_VIDEO_INPUT_H__
#define __WUKONG_VIDEO_INPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "tal_dvp.h"
#include "tal_uvc.h"
#include "tkl_video_in.h"

typedef enum {
    TAL_CAMERA_TYPE_DVP,
    TAL_CAMERA_TYPE_UVC,
    TAL_CAMERA_TYPE_MAX
} TAL_CAMERA_TYPE_E;

typedef struct {
    TAL_CAMERA_TYPE_E type;
    union {
        TUYA_DVP_USR_CFG_T dvp;
        TAL_UVC_CFG_T uvc;
    };
} TAL_CAMERA_CFG_T;

OPERATE_RET tal_camera_init(TAL_CAMERA_CFG_T *cfg);
OPERATE_RET tal_camera_deinit(VOID);
OPERATE_RET tal_camera_switch_to_h264_mode(VOID);
OPERATE_RET tal_camera_switch_to_jpeg_mode(VOID);

#ifdef __cplusplus
}
#endif

#endif