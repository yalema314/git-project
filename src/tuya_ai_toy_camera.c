#include "tuya_ai_toy_camera.h"
#include "tal_camera.h"
#include "tuya_device_cfg.h"
#include "tal_log.h"
#include "tal_semaphore.h"
#include "tal_mutex.h"
#include "tal_memory.h"

#if defined(ENABLE_MQTT_P2P) && (ENABLE_MQTT_P2P == 1)
#include "tkl_audio.h"
#include "tuya_ipc_media.h"
#include "tuya_ipc_media_stream.h"
#endif

#if defined(ENABLE_TUYA_UI) && ENABLE_TUYA_UI == 1       
#include "tkl_dma2d.h"
#include "lv_conf.h"
#include "ty_frame_buff.h"
#include "tuya_ai_display.h"
#include "tuya_lcd_pbuf.h"
#endif

#if defined(T5AI_BOARD_ROBOT) && T5AI_BOARD_ROBOT==1
#include "tal_uvc.h"
#endif

// JPEG帧获取结构
typedef struct {
    BYTE_T *data;
    UINT_T len;
    BOOL_T need_display;      // 是否需要在屏幕上显示
    BOOL_T need_capture;      // 是否需要捕获下一帧
    SEM_HANDLE sem;           // 信号量用于同步
    MUTEX_HANDLE mutex;
    SEM_HANDLE dma2d_sem;     // 信号量用于同步
    UINT8_T *lcd_buf;         // lcd flash
} JPEG_FRAME_CAPTURE_T;

STATIC JPEG_FRAME_CAPTURE_T s_jpeg_capture = {
    .data = NULL,
    .len = 0,
    .need_display = FALSE,
    .need_capture = FALSE,
    .sem = NULL,
    .mutex = NULL,
    .dma2d_sem = NULL,
    .lcd_buf = NULL,
};

STATIC TKL_VI_CAMERA_TYPE_E s_camera_type = TKL_VI_CAMERA_TYPE_DVP;

#if defined(ENABLE_TUYA_UI) && ENABLE_TUYA_UI == 1
STATIC TKL_DMA2D_FRAME_INFO_T in_frame = {0};
STATIC TKL_DMA2D_FRAME_INFO_T out_frame = {0};
STATIC INT_T camera_width = 0;
STATIC INT_T camera_height = 0;
STATIC INT_T lcd_width = 0;
STATIC INT_T lcd_height = 0;
STATIC TUYA_FRAME_FMT_E lcd_type = TUYA_FRAME_FMT_RGB565;

STATIC VOID_T __lcd_rgb565_swap_inplace(UINT8_T *buf, UINT32_T pixel_count)
{
    UINT16_T *pixel = (UINT16_T *)buf;

    if (!buf) {
        return;
    }

    for (UINT32_T i = 0; i < pixel_count; i++) {
        pixel[i] = ((pixel[i] & 0xff00) >> 8) | ((pixel[i] & 0x00ff) << 8);
    }
}

STATIC VOID_T __lcd_frame_flush_callback(UINT8_T *frame_data)
{
    ty_frame_buffer_t *frame = (ty_frame_buffer_t *)frame_data;

    if (frame && frame->pdata) {
        tuya_lcd_pbuf_node_t *pbuf_node = (tuya_lcd_pbuf_node_t *)frame->pdata;
        tuya_lcd_pbuf_release(pbuf_node);
        TAL_PR_TRACE("LCD frame flush complete, buffer released");
    }
}

STATIC VOID_T __flush_lcd_buffer(tuya_lcd_pbuf_node_t *lcd_buffer)
{
    if (!lcd_buffer) {
        return;
    }

    lcd_buffer->frame.x_start = 0;
    lcd_buffer->frame.y_start = 0;
    lcd_buffer->frame.width = lcd_width;
    lcd_buffer->frame.height = lcd_height;
    lcd_buffer->frame.len = lcd_width * lcd_height * 2;
    lcd_buffer->frame.pdata = (UINT8_T *)lcd_buffer;
    lcd_buffer->frame.free_cb = __lcd_frame_flush_callback;
    tuya_ai_display_flush(&lcd_buffer->frame);
}

STATIC VOID_T __to_lcd(TUYA_DVP_FRAME_MANAGE_T *output_frame)
{
    tuya_lcd_pbuf_node_t *lcd_buffer = NULL;
    OPERATE_RET rt = OPRT_OK;
    UINT16_T copy_width = 0;
    UINT16_T copy_height = 0;

    if (!output_frame || !output_frame->data) {
        TAL_PR_ERR("DVP frame invalid");
        return;
    }

    lcd_buffer = tuya_lcd_pbuf_acquire(0);
    if (!lcd_buffer) {
        TAL_PR_WARN("LCD buffer busy, drop DVP frame");
        return;
    }

    copy_width = (camera_width > lcd_width) ? lcd_width : camera_width;
    copy_height = (camera_height > lcd_height) ? lcd_height : camera_height;
    memset(lcd_buffer->frame.frame, 0, lcd_width * lcd_height * 2);

    in_frame.type = TUYA_FRAME_FMT_YUV422;
    in_frame.width = camera_width;
    in_frame.height = camera_height;
    in_frame.axis.x_axis = (camera_width > copy_width) ? ((camera_width - copy_width) / 2) : 0;
    in_frame.axis.y_axis = (camera_height > copy_height) ? ((camera_height - copy_height) / 2) : 0;
    in_frame.width_cp = copy_width;
    in_frame.height_cp = copy_height;
    in_frame.pbuf = (CHAR_T *)output_frame->data;

    out_frame.type = lcd_type;
    out_frame.width = lcd_width;
    out_frame.height = lcd_height;
    out_frame.axis.x_axis = 0;
    out_frame.axis.y_axis = 0;
    out_frame.width_cp = copy_width;
    out_frame.height_cp = copy_height;
    out_frame.pbuf = (CHAR_T *)lcd_buffer->frame.frame;

    rt = tkl_dma2d_convert(&in_frame, &out_frame);
    if (rt != OPRT_OK) {
        TAL_PR_ERR("DMA2D convert failed: %d", rt);
        tuya_lcd_pbuf_release(lcd_buffer);
        return;
    }

    rt = tal_semaphore_wait(s_jpeg_capture.dma2d_sem, 1000);
    if (rt != OPRT_OK) {
        TAL_PR_ERR("DMA2D wait timeout: %d", rt);
        tuya_lcd_pbuf_release(lcd_buffer);
        return;
    }

#if defined(T5AI_BOARD) && T5AI_BOARD == 1
    __lcd_rgb565_swap_inplace(lcd_buffer->frame.frame, lcd_width * lcd_height);
#endif

    __flush_lcd_buffer(lcd_buffer);
}

STATIC VOID_T __dma2d_irq_cb(TUYA_DMA2D_IRQ_E type, VOID_T *args)
{
    if (type == TUYA_DMA2D_TRANS_COMPLETE_ISR && args) {
        SEM_HANDLE tmp_sem = *((SEM_HANDLE *)args);
        if (tmp_sem) {
            tal_semaphore_post(tmp_sem);
        }
    } else if (type == TUYA_DMA2D_TRANS_ERROR_ISR) {
        TAL_PR_ERR("DMA2D transfer error");
    }
}
#endif

#if defined(ENABLE_MQTT_P2P) && (ENABLE_MQTT_P2P == 1)

STATIC INT_T __to_p2p(TUYA_DVP_FRAME_MANAGE_T *pframe)
{
    int rt = 0;
    if(tuya_ipc_get_client_online_num() <= 0)
    {
        return OPRT_RESOURCE_NOT_READY ;
    }
    MEDIA_FRAME_T frame;
    frame.type = pframe->is_i_frame ? E_VIDEO_I_FRAME : E_VIDEO_PB_FRAME;
    frame.p_buf = pframe->data;
    frame.size = pframe->data_len;

    frame.pts = tuya_p2p_misc_get_current_time_ms();
    frame.timestamp = tuya_p2p_misc_get_current_time_ms();
    rt = tuya_p2p_sdk_put_video_frame(&frame);
    TAL_PR_TRACE("__h264_cb frame size %d, rt = %d", pframe->data_len, rt);

    return 0;
}
#endif

STATIC VOID __camera_frame_handle_jpeg(JPEG_FRAME_CAPTURE_T *jpeg, UINT8_T *data, UINT32_T len)
{
    if (!jpeg || !data || len == 0) {
        TAL_PR_ERR("JPEG capture param invalid");
        return;
    }

    if (jpeg->mutex && jpeg->need_capture) {
        tal_mutex_lock(jpeg->mutex);
        
        if (jpeg->need_capture) {
            // 释放旧的缓存
            if (jpeg->data) {
                tal_free(jpeg->data);
                jpeg->data = NULL;
            }       
            
            // 分配新的缓存并复制数据
            jpeg->data = tal_malloc(len);
            if (jpeg->data) {
                memcpy(jpeg->data, data, len);
                jpeg->len = len;
                jpeg->need_capture = FALSE;
                
                // 通知等待线程数据已就绪
                if (jpeg->sem) {
                    tal_semaphore_post(jpeg->sem);
                }
                TAL_PR_DEBUG("JPEG frame captured, len: %d", len);
            } else {
                TAL_PR_ERR("Failed to allocate memory for JPEG frame");
                jpeg->len = 0;
            }
        }
        
        tal_mutex_unlock(jpeg->mutex);
    }
}


STATIC VOID __dvp_camera_frame_handle_local(TUYA_DVP_FRAME_MANAGE_T *output_frame)
{
    TUYA_FRAME_FMT_E fmt = output_frame->frame_fmt;
    switch (fmt)
    {
        case TUYA_FRAME_FMT_H264:
#if defined(ENABLE_AI_MODE_P2P) && (ENABLE_AI_MODE_P2P == 1)
#if defined(ENABLE_MQTT_P2P) && (ENABLE_MQTT_P2P == 1)
            __to_p2p(output_frame);
#endif
#endif
            break;
        case TUYA_FRAME_FMT_YUV422: // shown on the LCD
#if defined(T5AI_BOARD) && T5AI_BOARD==1
#if defined(ENABLE_TUYA_UI) && ENABLE_TUYA_UI == 1       
            if (s_jpeg_capture.mutex && s_jpeg_capture.need_display) {
                __to_lcd(output_frame);
            }
#endif     
#endif       
            break;
        case TUYA_FRAME_FMT_JPEG: // take a photo
            // TAL_PR_DEBUG("jpeg frame received, width %d height %d len:%d",output_frame->width, output_frame->height, output_frame->data_len);
            // 只在需要捕获时保存数据
            __camera_frame_handle_jpeg(&s_jpeg_capture, output_frame->data, output_frame->data_len);
            break;
        default:
            break;
    }
}

STATIC TUYA_CAMERA_OUTPUT_MODE __get_camera_output_mode(AI_TOY_CAMERA_TYPE_E type)
{
    switch (type)
    {
    case AI_TOY_CAMERA_LOCAL:
        TAL_PR_DEBUG("local camera, output mode is TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH");
        return TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH;    
    case AI_TOY_CAMERA_P2P:
        TAL_PR_DEBUG("p2p camera, output mode is TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH");
        return TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH;
    case AI_TOY_CAMERA_LAN:
        TAL_PR_DEBUG("lan camera, output mode is TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH");
        return TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH;
    }
}

STATIC DVP_FRAME_HANDLE __get_camera_handler(AI_TOY_CAMERA_TYPE_E type)
{
    switch (type)
    {
    case AI_TOY_CAMERA_LOCAL:
        return __dvp_camera_frame_handle_local;    
    default: 
        // TODO
        return NULL;
    }
}

#if defined(T5AI_BOARD_ROBOT) && T5AI_BOARD_ROBOT == 1
STATIC VOID __uvc_camera_frame_handle_local(TAL_UVC_HANDLE_T handle, TAL_UVC_FRAME_T *frame, VOID *args)
{
    if (!frame || !frame->frame || frame->length == 0) {
        TAL_PR_ERR("uvc camera frame handle param invalid");
        return;
    }

    TAL_PR_TRACE("uvc camera frame handle, fmt=%d width=%d height=%d len=%d", 
                 frame->fmt, frame->width, frame->height, frame->length);

    TUYA_FRAME_FMT_E fmt = frame->fmt;
    switch (fmt)
    {
        case TUYA_FRAME_FMT_RGB565:
#if defined(ENABLE_TUYA_UI) && ENABLE_TUYA_UI == 1
        {
            SYS_TICK_T start, end;
            tuya_lcd_pbuf_node_t *lcd_buffer = NULL;
            UINT16_T copy_width = 0;
            UINT16_T copy_height = 0;
            UINT16_T src_x_offset = 0;
            UINT16_T src_y_offset = 0;
            
            lcd_buffer = tuya_lcd_pbuf_acquire(0);
            if (!lcd_buffer) {
                TAL_PR_WARN("Failed to acquire LCD buffer, frame dropped");
                break;
            }
            
            start = tal_system_get_tick_count();
            memset(lcd_buffer->frame.frame, 0, lcd_width * lcd_height * 2);

            copy_width = (frame->width > lcd_width) ? lcd_width : frame->width;
            copy_height = (frame->height > lcd_height) ? lcd_height : frame->height;
            src_x_offset = (frame->width > copy_width) ? ((frame->width - copy_width) / 2) : 0;
            src_y_offset = (frame->height > copy_height) ? ((frame->height - copy_height) / 2) : 0;

            uint16_t *src_buf = (uint16_t *)frame->frame;
            uint16_t *dst_buf = (uint16_t *)lcd_buffer->frame.frame;

            for (UINT16_T row = 0; row < copy_height; row++) {
                uint16_t *src_row = src_buf + ((src_y_offset + row) * frame->width) + src_x_offset;
                uint16_t *dst_row = dst_buf + (row * lcd_width);

                for (UINT16_T col = 0; col < copy_width; col++) {
                    dst_row[col] = ((src_row[col] & 0xff00) >> 8) | ((src_row[col] & 0x00ff) << 8);
                }
            }
            end = tal_system_get_tick_count();
            TAL_PR_TRACE("Copy & byte swap: %d ticks, %ux%u", end - start, copy_width, copy_height);
            __flush_lcd_buffer(lcd_buffer);
        }
#endif
            break;

        case TUYA_FRAME_FMT_JPEG:
            __camera_frame_handle_jpeg(&s_jpeg_capture, frame->frame, frame->length);
            break;
        default:
            break;
    }
}
#endif

/**
 * @brief 获取一帧JPEG图像数据(拍照)
 * @param[out] data 输出JPEG数据指针,需要调用者使用tal_free释放
 * @param[out] len 输出JPEG数据长度
 * @param[in] user_data 用户自定义数据(暂未使用)
 * @return OPERATE_RET OPRT_OK表示成功,其他表示失败
 */
OPERATE_RET tuya_ai_toy_camera_get_jpeg_frame(BYTE_T **data, UINT_T *len, VOID *user_data)
{
    if (!data || !len) {
        TAL_PR_ERR("JPEG capture param invalid");
        return OPRT_INVALID_PARM;
    }
    
    if (!s_jpeg_capture.mutex || !s_jpeg_capture.sem) {
        TAL_PR_ERR("JPEG capture not initialized");
        return OPRT_COM_ERROR;
    }
    
    // 设置捕获标志
    tal_mutex_lock(s_jpeg_capture.mutex);
    s_jpeg_capture.need_capture = TRUE;
    tal_mutex_unlock(s_jpeg_capture.mutex);
    
    
    TAL_PR_DEBUG("Waiting for next JPEG frame...");
    
    // 等待帧数据就绪,超时时间3秒
    OPERATE_RET ret = tal_semaphore_wait(s_jpeg_capture.sem, 3000);
    if (ret != OPRT_OK) {
        tal_mutex_lock(s_jpeg_capture.mutex);
        s_jpeg_capture.need_capture = FALSE;
        tal_mutex_unlock(s_jpeg_capture.mutex);
        TAL_PR_ERR("Wait for JPEG frame timeout");
        return OPRT_COM_ERROR;
    }
    
    // 获取数据
    tal_mutex_lock(s_jpeg_capture.mutex);
    
    if (!s_jpeg_capture.data || s_jpeg_capture.len == 0) {
        tal_mutex_unlock(s_jpeg_capture.mutex);
        TAL_PR_ERR("No valid JPEG frame data");
        return OPRT_COM_ERROR;
    }
    
    // 分配内存并复制数据给调用者
    *data = (BYTE_T *)tal_malloc(s_jpeg_capture.len);
    if (!(*data)) {
        tal_mutex_unlock(s_jpeg_capture.mutex);
        TAL_PR_ERR("Failed to allocate memory for JPEG frame");
        return OPRT_MALLOC_FAILED;
    }
    
    memcpy(*data, s_jpeg_capture.data, s_jpeg_capture.len);
    *len = s_jpeg_capture.len;
    
    tal_mutex_unlock(s_jpeg_capture.mutex);
    
    TAL_PR_DEBUG("Get JPEG frame success, len: %d", *len);
    return OPRT_OK;
}

OPERATE_RET tuya_ai_toy_camera_init(AI_TOY_CAMERA_TYPE_E type)
{
    OPERATE_RET rt = OPRT_OK;
    TAL_CAMERA_CFG_T video = {0};
#if defined(ENABLE_TUYA_UI) && ENABLE_TUYA_UI == 1
    OPERATE_RET pbuf_ret = OPRT_OK;
#endif

#if defined(T5AI_BOARD_ROBOT) && T5AI_BOARD_ROBOT==1
    //! UVC camera configuration
    video.type = TKL_VI_CAMERA_TYPE_UVC;
    video.uvc.fmt = TUYA_FRAME_FMT_RGB565;
    video.uvc.width = 320;
    video.uvc.height = 240;
    video.uvc.fps = 15;
    video.uvc.power_pin = TUYA_GPIO_NUM_6;
    video.uvc.active_level = TUYA_GPIO_LEVEL_HIGH;
    video.uvc.frame_cb = __uvc_camera_frame_handle_local;
    video.uvc.args = NULL;
    camera_width = video.uvc.width;
    camera_height = video.uvc.height;
#else
    video.type = TKL_VI_CAMERA_TYPE_DVP;
    video.dvp.dvp_cfg.fps = 20;
    video.dvp.dvp_cfg.width = 480;
    video.dvp.dvp_cfg.height = 480;
    video.dvp.dvp_cfg.output_mode = __get_camera_output_mode(type);
    video.dvp.dvp_cfg.sync_polarity = 0; 
    video.dvp.dvp_frame_handle = __get_camera_handler(type);
    video.dvp.dvp_cfg.encoded_quality.jpeg_cfg.enable = TRUE; 
    video.dvp.dvp_cfg.encoded_quality.jpeg_cfg.min_size = 10;
    video.dvp.dvp_cfg.encoded_quality.jpeg_cfg.max_size = 25;
    video.dvp.pin_cfg.dvp_i2c_idx = TUYA_I2C_NUM_1;  
    video.dvp.pin_cfg.dvp_i2c_clk.pin = TUYA_GPIO_NUM_13; 
    video.dvp.pin_cfg.dvp_i2c_sda.pin = TUYA_GPIO_NUM_15;  
    video.dvp.pin_cfg.dvp_rst_ctrl.pin = TUYA_GPIO_NUM_51;  
    video.dvp.pin_cfg.dvp_rst_ctrl.active_level = TUYA_GPIO_LEVEL_LOW;  
    video.dvp.pin_cfg.dvp_pwr_ctrl.pin = TUYA_GPIO_NUM_MAX;
    camera_width = video.dvp.dvp_cfg.width;
    camera_height = video.dvp.dvp_cfg.height;
#endif
    lcd_width = TUYA_LCD_WIDTH;
    lcd_height = TUYA_LCD_HEIGHT;

#if defined(ENABLE_TUYA_UI) && ENABLE_TUYA_UI == 1
    pbuf_ret = tuya_lcd_pbuf_init(lcd_width, lcd_height, 3);
    if (pbuf_ret != OPRT_OK) {
        TAL_PR_ERR("Failed to init LCD pbuf pool: %d", pbuf_ret);
        goto __error_exit;
    }
    TAL_PR_DEBUG("LCD pbuf pool initialized: 3 buffers (%dx%d)", lcd_width, lcd_height);
#endif
    TUYA_CALL_ERR_LOG(tal_camera_init(&video));   

    s_camera_type = video.type;

    // 初始化JPEG帧捕获同步机制
    if (!s_jpeg_capture.mutex) {
        tal_mutex_create_init(&s_jpeg_capture.mutex);
    }
    if (!s_jpeg_capture.sem) {
        tal_semaphore_create_init(&s_jpeg_capture.sem, 0, 1);
    }

#if defined(T5AI_BOARD) && T5AI_BOARD==1
    if (!s_jpeg_capture.dma2d_sem) {
        tal_semaphore_create_init(&s_jpeg_capture.dma2d_sem, 0, 1);
    }
#endif
    switch (LV_COLOR_DEPTH) {
    case 16:
        lcd_type = TUYA_FRAME_FMT_RGB565;
        break;
    case 18:
        lcd_type = TUYA_FRAME_FMT_RGB565;
        break;
    case 24:
        lcd_type = TUYA_FRAME_FMT_RGB888;
        break;
    }
    TAL_PR_DEBUG("ai toy -> camera init success!");
    return rt; 
__error_exit:
    tuya_ai_toy_camera_deinit();
    return rt;
}


OPERATE_RET tuya_ai_toy_camera_deinit()
{
#if defined(ENABLE_TUYA_UI) && ENABLE_TUYA_UI == 1
    // Cleanup LCD buffer pool
    tuya_lcd_pbuf_deinit();
#endif

    // 清理JPEG帧捕获资源
    if (s_jpeg_capture.mutex) {
        tal_mutex_lock(s_jpeg_capture.mutex);
        if (s_jpeg_capture.data) {
            tal_free(s_jpeg_capture.data);
            s_jpeg_capture.data = NULL;
        }
        s_jpeg_capture.len = 0;
        s_jpeg_capture.need_display = FALSE;
        s_jpeg_capture.need_capture = FALSE;
        tal_mutex_unlock(s_jpeg_capture.mutex);
        tal_mutex_release(s_jpeg_capture.mutex);
        s_jpeg_capture.mutex = NULL;
    }

    if (s_jpeg_capture.sem) {
        tal_semaphore_release(s_jpeg_capture.sem);
        s_jpeg_capture.sem = NULL;
    }    

    if (s_jpeg_capture.dma2d_sem) {
        tal_semaphore_release(s_jpeg_capture.dma2d_sem);
        s_jpeg_capture.dma2d_sem = NULL;
    }    

    if (s_jpeg_capture.lcd_buf) {
        tal_free(s_jpeg_capture.lcd_buf);
        s_jpeg_capture.lcd_buf = NULL;
    }

    tal_camera_deinit();

    return OPRT_OK;
}

OPERATE_RET tuya_ai_toy_camera_start(VOID)
{
    tal_mutex_lock(s_jpeg_capture.mutex);
    tuya_ai_display_pause();
    s_jpeg_capture.need_display = TRUE;
#if defined(T5AI_BOARD) && T5AI_BOARD==1
    VOID_T *tmp_arg = (VOID_T *)(&s_jpeg_capture.dma2d_sem);
    TUYA_DMA2D_BASE_CFG_T dma2d_cfg = {.cb = __dma2d_irq_cb, .arg=tmp_arg};
    tkl_dma2d_init(&dma2d_cfg);
#elif defined(T5AI_BOARD_ROBOT) && T5AI_BOARD_ROBOT==1
    //! TODO:
    tal_uvc_start(NULL);
#endif
    tal_mutex_unlock(s_jpeg_capture.mutex);

    return OPRT_OK;
}

OPERATE_RET tuya_ai_toy_camera_stop(VOID)
{
    tal_mutex_lock(s_jpeg_capture.mutex);
#if defined(T5AI_BOARD) && T5AI_BOARD==1
    tkl_dma2d_deinit();
#elif defined(T5AI_BOARD_ROBOT) && T5AI_BOARD_ROBOT==1
    //! TODO:
    tal_uvc_stop(NULL);
#endif
    tuya_ai_display_resume();
    s_jpeg_capture.need_display = FALSE;
    tal_mutex_unlock(s_jpeg_capture.mutex);    

    return OPRT_OK;
}
