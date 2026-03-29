#include "jpeg_hw_decode.h"

#ifdef ENABLE_JPEG_IMAGE_DECODE_WITH_HW
#include "img_utility.h"
#include "os/os.h"

#include "lvgl.h"
#include "os/mem.h"
#include "driver/lcd.h"
#include <soc/mapping.h>
#include "driver/media_types.h"
#include "modules/jpeg_decode_sw.h"
#include "driver/jpeg_dec_types.h"
#include "mux_pipeline.h"
#include "driver/dma2d.h"
#include "modules/image_scale.h"
#include <driver/jpeg_dec.h>
#include "bk_posix.h"

static beken_semaphore_t g_hw_decode_sem = NULL;
static beken2_timer_t g_hw_decode_timer;
static u8 *g_dec_frame_data = NULL;

extern void tkl_system_psram_free(void* ptr);
extern void* tkl_system_psram_malloc(const size_t size);

static void _jpeg_dec_err_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;

    rtos_stop_oneshot_timer(&g_hw_decode_timer);

    TY_GUI_LOG_PRINT("%s:%x \n", __func__,result->ok);
    ret = rtos_set_semaphore(&g_hw_decode_sem);
    if (ret != BK_OK)
    {
        TY_GUI_LOG_PRINT("%s semaphore set failed: %d\n", __func__, ret);
    }
}

static void _jpeg_dec_eof_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;

    rtos_stop_oneshot_timer(&g_hw_decode_timer);

    if (result->ok == false)
    {
        TY_GUI_LOG_PRINT("%s decoder error\n", __func__);
    }
    else
    {
//        TY_GUI_LOG_PRINT("%s decoder success\n", __func__);
    }

    ret = rtos_set_semaphore(&g_hw_decode_sem);
    if (ret != BK_OK)
    {
        TY_GUI_LOG_PRINT("%s semaphore set failed: %d\n", __func__, ret);
    }
}

static void _hw_decode_timeout(void *Larg, void *Rarg)
{
    bk_err_t ret = BK_FAIL;

    TY_GUI_LOG_PRINT("%s \n", __func__);
    ret = rtos_set_semaphore(&g_hw_decode_sem);
    if (ret != BK_OK)
    {
        TY_GUI_LOG_PRINT("%s semaphore set failed: %d\n", __func__, ret);
    }
}

extern uint8_t lv_dma2d_use_flag;
static void dma2d_yuyv2rgb565(char *src, char *dst,    uint16_t width,    uint16_t height )
{
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};
    //static int flag = 0;

    dma2d_memcpy_pfc.input_addr = (char *)src;
    dma2d_memcpy_pfc.output_addr = (char *)dst;
    dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dma2d_width = width;
    dma2d_memcpy_pfc.dma2d_height = height;
    dma2d_memcpy_pfc.src_frame_width = width;
    dma2d_memcpy_pfc.src_frame_height = height;
    dma2d_memcpy_pfc.dst_frame_width = width;
    dma2d_memcpy_pfc.dst_frame_height = height;
    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = 0;
    dma2d_memcpy_pfc.dst_frame_ypos = 0;
    dma2d_memcpy_pfc.input_red_blue_swap = 0;
    dma2d_memcpy_pfc.output_red_blue_swap = 0;

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();
    lv_dma2d_use_flag = 1;
    extern void lv_dma2d_memcpy_wait_transfer_finish(void);
    lv_dma2d_memcpy_wait_transfer_finish();
}

OPERATE_RET jpeg_hw_decode(void *_jpeg_frame, void *_img_dst)
{
    OPERATE_RET ret = BK_FAIL;
    //sw_jpeg_dec_res_t result;
    gui_img_frame_buffer_t *jpeg_frame = (gui_img_frame_buffer_t *)_jpeg_frame;
    lv_img_dsc_t *img_dst = (lv_img_dsc_t *)_img_dst;
    int flag = 0;

    do{
        ret = rtos_init_semaphore_ex(&g_hw_decode_sem, 1, 0);
        if (ret != BK_OK)
        {
            TY_GUI_LOG_PRINT("[%s][%d] init sem fail\n", __FUNCTION__, __LINE__);
            break;
        }
        
        if (!rtos_is_oneshot_timer_init(&g_hw_decode_timer))
        {
            ret = rtos_init_oneshot_timer(&g_hw_decode_timer, 1000, _hw_decode_timeout, NULL, NULL);
            if (ret != BK_OK)
            {
                TY_GUI_LOG_PRINT("[%s][%d] create timer fail\n", __FUNCTION__, __LINE__);
                break;
            }
            ret = rtos_start_oneshot_timer(&g_hw_decode_timer);
            if(ret != BK_OK)
            {
                TY_GUI_LOG_PRINT("[%s][%d] start timer fail\n", __FUNCTION__, __LINE__);
                break;
            }
        }
        else
        {
            ret = rtos_oneshot_reload_timer(&g_hw_decode_timer);
            if(ret != BK_OK)
            {
                TY_GUI_LOG_PRINT("[%s][%d] reload timer fail\n", __FUNCTION__, __LINE__);
                break;
            }
        }

        g_dec_frame_data = tkl_system_psram_malloc(img_dst->data_size);
        if(!g_dec_frame_data)
        {
            TY_GUI_LOG_PRINT("[%s][%d] malloc psram size %d fail\r\n", __FUNCTION__, __LINE__, g_dec_frame_data);
            ret = BK_ERR_NO_MEM;
            break;
        }
        
        bk_jpeg_dec_driver_init();
        bk_jpeg_dec_isr_register(DEC_ERR, _jpeg_dec_err_cb);
        bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, _jpeg_dec_eof_cb);

        bk_jpeg_dec_out_format(PIXEL_FMT_YUYV);
        ret = bk_jpeg_dec_hw_start(jpeg_frame->length, jpeg_frame->frame, g_dec_frame_data);
        if (ret != BK_OK)
        {
            TY_GUI_LOG_PRINT("%s hw decode start fail %d\n", __func__, ret);
            break;
        }

        ret = rtos_get_semaphore(&g_hw_decode_sem, 1000);
        if (ret != BK_OK)
        {
            TY_GUI_LOG_PRINT("%s semaphore get failed: %d\n", __func__, ret);
            break;
        }

        if(img_dst->data == NULL)
        {
            img_dst->data = tkl_system_psram_malloc(img_dst->data_size);
            if(!img_dst->data)
            {
                TY_GUI_LOG_PRINT("[%s][%d] malloc psram size %d fail\r\n", __FUNCTION__, __LINE__, img_dst->data_size);
                ret = BK_ERR_NO_MEM;
                break;
            }

            flag = 1;
        }

        if(0/*2 == lv_vendor_display_frame_cnt() || 2 == lv_vendor_draw_buffer_cnt()*/)
        {
            yuyv_to_rgb565(g_dec_frame_data,(uint8_t *)img_dst->data,(int)img_dst->header.w,(int)img_dst->header.h);//25.4ms
        }
        else
        {
            //rtos_delay_milliseconds(10);//param 4 fail,6 ok OR fail ????
            dma2d_yuyv2rgb565((char *)g_dec_frame_data,(char *)img_dst->data,img_dst->header.w,img_dst->header.h);//9.3ms
        }

#if LVGL_VERSION_MAJOR < 9
        img_dst->header.always_zero = 0;
#else
        img_dst->header.magic = LV_IMAGE_HEADER_MAGIC;
#endif
        img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
        ret = BK_OK;
    }while(0);

    if (rtos_is_oneshot_timer_init(&g_hw_decode_timer))
    {
        rtos_deinit_oneshot_timer(&g_hw_decode_timer);
    }

    if(g_hw_decode_sem)
    {
        rtos_deinit_semaphore(&g_hw_decode_sem);
        g_hw_decode_sem = NULL;
    }
    
    if(g_dec_frame_data)
    {
        tkl_system_psram_free(g_dec_frame_data); 
        g_dec_frame_data  = NULL;
    }
    
    bk_jpeg_dec_stop();
    bk_jpeg_dec_driver_deinit();
    
    if(BK_OK != ret)
    {
        if(flag)
        {
            tkl_system_psram_free((void *)img_dst->data);
            img_dst->data = NULL;
        }
    }

    return ret;
}
#else
OPERATE_RET jpeg_hw_decode(void *_jpeg_frame, void *_img_dst)
{
    return OPRT_COM_ERROR;
}
#endif