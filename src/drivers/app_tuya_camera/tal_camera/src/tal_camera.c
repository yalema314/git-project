#include "tal_camera.h"
#include "tuya_ai_client.h"
#include "tal_workqueue.h"
#include "tal_workq_service.h"
#include "tal_log.h"
#include "tal_semaphore.h"
#include "base_event.h"
#include "tkl_video_in.h"
#include "tal_dvp.h"
#include "tal_uvc.h"

extern TUYA_DVP_SENSOR_CFG_T dvp_sensor_gc2145_cfg;
STATIC TAL_CAMERA_CFG_T *__s_video_config = NULL;
STATIC VOID *cur_camera_device = NULL;

STATIC VOID __dvp_close(VOID)
{
    if (cur_camera_device)
    {
        tal_dvp_deinit(cur_camera_device);
        cur_camera_device = NULL;
    }

    return;
}

OPERATE_RET tal_camera_switch_to_h264_mode(VOID)
{
    __dvp_close();
    __s_video_config->dvp.dvp_cfg.output_mode = TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH;

    TUYA_DVP_DEVICE_T *dvp_device = tal_dvp_init(&dvp_sensor_gc2145_cfg, &__s_video_config->dvp);
    if (!dvp_device)
    {
        goto __error_exit;
    }

    cur_camera_device = dvp_device;

    TAL_PR_DEBUG("wukong vedio -> switch to h264 mode success");
    return OPRT_OK;
__error_exit:
    tal_camera_deinit();
    return OPRT_COM_ERROR;
}

OPERATE_RET tal_camera_switch_to_jpeg_mode(VOID)
{
    __dvp_close();
    __s_video_config->dvp.dvp_cfg.output_mode = TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH;
    TUYA_DVP_DEVICE_T *dvp_device = tal_dvp_init(&dvp_sensor_gc2145_cfg, &__s_video_config->dvp);
    if (!dvp_device)
    {
        goto __error_exit;
    }
    cur_camera_device = dvp_device;
    TAL_PR_DEBUG("wukong vedio -> switch to jpeg mode success");
    return OPRT_OK;
__error_exit:
    tal_camera_deinit();
    return OPRT_COM_ERROR;
}

STATIC VOID __camera_init_workq(VOID_T *args)
{
    TAL_PR_DEBUG("wukong vedio -> camera init event");
    TAL_CAMERA_CFG_T *cfg = (TAL_CAMERA_CFG_T*)args;

    if (cfg->type == TKL_VI_CAMERA_TYPE_DVP) {
        TUYA_DVP_DEVICE_T *dvp_device = tal_dvp_init(&dvp_sensor_gc2145_cfg, &cfg->dvp);

        if (!dvp_device) {
            goto __error_exit;
        }
        
        TAL_PR_DEBUG("wukong vedio -> dvp init success"); 
        cur_camera_device = dvp_device;
    } else if (cfg->type == TKL_VI_CAMERA_TYPE_UVC) {
        tal_uvc_init((TAL_UVC_HANDLE_T*)&cur_camera_device, &cfg->uvc);
        TAL_PR_DEBUG("wukong vedio -> uvc init success"); 
    }
    return;

__error_exit:
    tal_camera_deinit();
    return;
}

STATIC DELAYED_WORK_HANDLE delayed_work = NULL;
STATIC INT_T __on_client_run_event(void *args)
{
    if(delayed_work != NULL) {
        return 0;
    }
    tal_workq_init_delayed(WORKQ_SYSTEM, __camera_init_workq,  __s_video_config, &delayed_work);
    tal_workq_start_delayed(delayed_work, 500, LOOP_ONCE);
    return 0;
}

OPERATE_RET tal_camera_init(TAL_CAMERA_CFG_T *cfg)
{   
    TUYA_CHECK_NULL_RETURN(cfg, OPRT_INVALID_PARM);

    if (__s_video_config) {
        tal_camera_deinit();
    }

#ifdef ENABLE_EXT_RAM
    __s_video_config = (TAL_CAMERA_CFG_T*)tal_psram_malloc(sizeof(TAL_CAMERA_CFG_T));
#else
    __s_video_config = (TAL_CAMERA_CFG_T*)tal_malloc(sizeof(TAL_CAMERA_CFG_T));
#endif
    memcpy(__s_video_config, cfg, sizeof(TAL_CAMERA_CFG_T));
    __on_client_run_event(NULL);

    return OPRT_OK;
}

OPERATE_RET tal_camera_deinit(VOID)
{
    if (__s_video_config) {
        if (cur_camera_device)
        {
            if (__s_video_config->type == TKL_VI_CAMERA_TYPE_DVP) {
                tal_dvp_deinit(cur_camera_device);
            } else if (__s_video_config->type == TKL_VI_CAMERA_TYPE_UVC) {
                // TODO
            }
            cur_camera_device = NULL;
        } 

        if (delayed_work != NULL) {
            tal_workq_cancel_delayed(delayed_work);
            delayed_work = NULL;
        }

        tal_free(__s_video_config);
        __s_video_config = NULL;
    }

    return OPRT_OK;
}