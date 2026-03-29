/**
 * @file libjpeg_turbo_decode.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "libjpeg_turbo_decode.h"

#if defined(TUYA_LIBJPEG_TURBO) && (TUYA_LIBJPEG_TURBO == 1)
#include <stdio.h>
#include <jpeglib.h>
#include <jpegint.h>
#include <setjmp.h>
#include "img_utility.h"
#include "tkl_memory.h"

/*********************
 *      DEFINES
 *********************/ 
//#define IMG_DECODING_TIME_TEST
#define JPEG_PIXEL_SIZE 3 /* RGB888 */
#define JPEG_SIGNATURE 0xFFD8FF
#define IS_JPEG_SIGNATURE(x) (((x) & 0x00FFFFFF) == JPEG_SIGNATURE)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct error_mgr_s {
    struct jpeg_error_mgr pub;
    jmp_buf jb;
} error_mgr_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static OPERATE_RET libjpeg_turbo_decode_jpeg_file(uint8_t * data, uint32_t data_size, uint8_t *out_data);
static bool libjpeg_turbo_get_jpeg_head_info(uint8_t * data, uint32_t data_size, uint32_t * width, uint32_t * height, uint32_t * orientation);
static bool libjpeg_turbo_get_jpeg_size(uint8_t * data, uint32_t data_size, uint32_t * width, uint32_t * height);
static bool libjpeg_turbo_get_jpeg_direction(uint8_t * data, uint32_t data_size, uint32_t * orientation);
static void libjpeg_turbo_error_exit(j_common_ptr cinfo);
static OPERATE_RET jpg_dec_img_start(gui_img_frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst);

/**********************
 *  STATIC VARIABLES
 **********************/
static const int JPEG_EXIF = 0x45786966; /* Exif data structure tag */
static const int JPEG_BIG_ENDIAN_TAG = 0x4d4d;
static const int JPEG_LITTLE_ENDIAN_TAG = 0x4949;

/**********************
 *      MACROS
 **********************/
#define TRANS_32_VALUE(big_endian, data) big_endian ? \
    ((*(data) << 24) | (*((data) + 1) << 16) | (*((data) + 2) << 8) | *((data) + 3)) : \
    (*(data) | (*((data) + 1) << 8) | (*((data) + 2) << 16) | (*((data) + 3) << 24))
#define TRANS_16_VALUE(big_endian, data) big_endian ? \
    ((*(data) << 8) | *((data) + 1)) : (*(data) | (*((data) + 1) << 8))

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#ifdef IMG_DECODING_TIME_TEST
extern unsigned long long int __current_timestamp(void);
#endif
OPERATE_RET jpg_dec_img(bool is_file, char *file_name, uint8_t *img_data, uint32_t img_size, lv_img_dsc_t *img_dst)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    gui_img_frame_buffer_t *jpeg_frame = NULL;
    unsigned long long int last_run_ms = 0, curr_run_ms = 0;

    do{
        if (is_file) {
        #ifdef IMG_DECODING_TIME_TEST
            last_run_ms = __current_timestamp();
        #endif
            jpeg_frame = img_read_file(file_name);
        #ifdef IMG_DECODING_TIME_TEST
            curr_run_ms = __current_timestamp();
        #endif
            TY_GUI_LOG_PRINT("[%s][%d] file '%s' get use '%llu'ms\r\n",__FUNCTION__, __LINE__, file_name,
                curr_run_ms-last_run_ms);
        }
        else
            jpeg_frame = img_read_raw_data(img_data, img_size);
        if (jpeg_frame == NULL)
        {
            ret = OPRT_COM_ERROR;
            break;
        }
    #ifdef IMG_DECODING_TIME_TEST
        last_run_ms = __current_timestamp();
    #endif
        ret = jpg_dec_img_start(jpeg_frame, img_dst);
    #ifdef IMG_DECODING_TIME_TEST
        curr_run_ms = __current_timestamp();
    #endif
        if(OPRT_OK == ret)
        {
            TY_GUI_LOG_PRINT("[%s][%d] sw decode success, width:%d, height:%d, size:%d, decode use time '%llu'ms\r\n", 
                            __FUNCTION__, __LINE__,
                            img_dst->header.w, img_dst->header.h, img_dst->data_size, curr_run_ms-last_run_ms);
        }
    }while(0);

    if (jpeg_frame)
    {
        if(jpeg_frame->frame && is_file)
        {
            tkl_system_psram_free(jpeg_frame->frame);
            jpeg_frame->frame = NULL;
        }
        
        tkl_system_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return ret;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static OPERATE_RET jpg_dec_img_start(gui_img_frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    unsigned char bytes_per_pixel = 2;
#if LV_COLOR_DEPTH == 16
    bytes_per_pixel = 2;
#elif LV_COLOR_DEPTH == 24
    bytes_per_pixel = 3;
#elif LV_COLOR_DEPTH == 32
    bytes_per_pixel = 3;
#else
#error "unknown color depth"
#endif

    uint32_t width;
    uint32_t height;
    uint32_t orientation = 0;

    if(!libjpeg_turbo_get_jpeg_head_info(jpeg_frame->frame, jpeg_frame->length, &width, &height, &orientation)) {
        TY_GUI_LOG_PRINT("[%s][%d] get head info fail\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    img_dst->header.w = (orientation % 180) ? height : width;
    img_dst->header.h = (orientation % 180) ? width : height;
    img_dst->data_size = img_dst->header.w*img_dst->header.h*bytes_per_pixel;
#if LVGL_VERSION_MAJOR >= 9
    img_dst->header.stride = img_dst->header.w*bytes_per_pixel;
#endif
    if(img_dst->data == NULL)
    {
        img_dst->data = tkl_system_psram_malloc(img_dst->data_size);
        if(!img_dst->data)
        {
            TY_GUI_LOG_PRINT("[%s][%d] malloc psram size %d fail\r\n", __FUNCTION__, __LINE__, img_dst->data_size);
            return ret;
        }
    }

/******************************/
    ret = libjpeg_turbo_decode_jpeg_file(jpeg_frame->frame, jpeg_frame->length, (uint8_t *)img_dst->data);
    if(ret != OPRT_OK) {
        TY_GUI_LOG_PRINT("decode jpeg file failed");
        return ret;
    }
/******************************/
    
#if LVGL_VERSION_MAJOR < 9
    img_dst->header.always_zero = 0;
    img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
#else
    //jpg_convert_color_depth(img_dst->data, img_dst->data_size/bytes_per_pixel);
    img_dst->header.magic= LV_IMAGE_HEADER_MAGIC;
    #if LV_COLOR_DEPTH == 32
        img_dst->header.cf = LV_COLOR_FORMAT_RGB888;        //最高支持RGB888
    #else
        img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
    #endif
#endif

    return ret;
}

static OPERATE_RET libjpeg_turbo_decode_jpeg_file(uint8_t * data, uint32_t data_size, uint8_t *out_data)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    unsigned char bytes_per_pixel = 2;
    /* This struct contains the JPEG decompression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     */
    struct jpeg_decompress_struct cinfo;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    error_mgr_t jerr;
    //lv_color_format_t cf = LV_COLOR_FORMAT_NATIVE;

    /* More stuff */
    JSAMPARRAY buffer;  /* Output row buffer */

    int row_stride;     /* physical row width in output buffer */
    uint32_t image_angle = 0;   /* image rotate angle */

    /* allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = libjpeg_turbo_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if(setjmp(jerr.jb)) {
        TY_GUI_LOG_PRINT("decoding error");
        /* If we get here, the JPEG code has signaled an error.
        * We need to clean up the JPEG object, close the input file, and return.
        */
        jpeg_destroy_decompress(&cinfo);
        ret = OPRT_COM_ERROR;
        return ret;
    }

    /* Get rotate angle from Exif data */
    //if(!libjpeg_turbo_get_jpeg_direction(data, data_size, &image_angle)) {
    //    TY_GUI_LOG_PRINT("read jpeg orientation failed.");
    //}

    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* specify data source (eg, a file or buffer) */

    jpeg_mem_src(&cinfo, data, data_size);

    /* read file parameters with jpeg_read_header() */

    jpeg_read_header(&cinfo, TRUE);

    /* We can ignore the return value from jpeg_read_header since
     *   (a) suspension is not possible with the stdio data source, and
     *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
     * See libjpeg.doc for more info.
     */

    /* set parameters for decompression */

#if LV_COLOR_DEPTH == 24 || LV_COLOR_DEPTH == 32
    cinfo.out_color_space = JCS_EXT_BGR;
    //cf = LV_COLOR_FORMAT_RGB888;
    bytes_per_pixel = 3;
#elif LV_COLOR_DEPTH == 16
    cinfo.out_color_space = JCS_RGB565;
    bytes_per_pixel = 2;
#elif LV_COLOR_DEPTH == 8
    cinfo.out_color_space = JCS_GRAYSCALE;
    bytes_per_pixel = 1;
#else
    #error "unknown color depth"
#endif
    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */

    /* Start decompressor */

    jpeg_start_decompress(&cinfo);
    TY_GUI_LOG_PRINT("[%s][%d]:bytes_per_pixel '%d', output_components '%d'", __func__, __LINE__, bytes_per_pixel, cinfo.output_components);
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* We may need to do some setup of our own at this point before reading
     * the data.  After jpeg_start_decompress() we have the correct scaled
     * output image dimensions available, as well as the output colormap
     * if we asked for color quantization.
     * In this example, we need to make an output work buffer of the right size.
     */
    /* JSAMPLEs per row in output buffer */
    //if (cinfo.out_color_space == JCS_RGB565)
    //    row_stride = cinfo.output_width * 2;
    //else
        row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)
             ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    uint32_t buf_width = (image_angle % 180) ? cinfo.output_height : cinfo.output_width;
    uint32_t buf_height = (image_angle % 180) ? cinfo.output_width : cinfo.output_height;
    uint32_t stride = buf_width * bytes_per_pixel;
    if(out_data != NULL) {
        uint32_t line_index = 0;
        /* while (scan lines remain to be read) */
        /* jpeg_read_scanlines(...); */

        /* Here we use the library's state variable cinfo.output_scanline as the
         * loop counter, so that we don't have to keep track ourselves.
         */
        while(cinfo.output_scanline < cinfo.output_height) {
            /* jpeg_read_scanlines expects an array of pointers to scanlines.
             * Here the array is only one element long, but you could ask for
             * more than one scanline at a time if that's more convenient.
             */
            jpeg_read_scanlines(&cinfo, buffer, 1);

            /* Assume put_scanline_someplace wants a pointer and sample count. */
            memcpy(out_data + line_index * stride, buffer[0], stride);
            line_index++;
        }
    }

    /* Finish decompression */

    jpeg_finish_decompress(&cinfo);

    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

    /* And we're done! */
    ret = OPRT_OK;
    return ret;
}

static bool libjpeg_turbo_get_jpeg_head_info(uint8_t * data, uint32_t data_size, uint32_t * width, uint32_t * height, uint32_t * orientation)
{
    if(data == NULL) {
        return false;
    }

    if(!libjpeg_turbo_get_jpeg_size(data, data_size, width, height)) {
        TY_GUI_LOG_PRINT("read jpeg size failed.");
    }

    //if(!libjpeg_turbo_get_jpeg_direction(data, data_size, orientation)) {
    //    TY_GUI_LOG_PRINT("read jpeg orientation failed.");
    //}
    return true;
}

static bool libjpeg_turbo_get_jpeg_size(uint8_t * data, uint32_t data_size, uint32_t * width, uint32_t * height)
{
    struct jpeg_decompress_struct cinfo;
    error_mgr_t jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = libjpeg_turbo_error_exit;

    if(setjmp(jerr.jb)) {
        TY_GUI_LOG_PRINT("read jpeg head failed");
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, data, data_size);

    int ret = jpeg_read_header(&cinfo, TRUE);

    if(ret == JPEG_HEADER_OK) {
        *width = cinfo.image_width;
        *height = cinfo.image_height;
    }
    else {
        TY_GUI_LOG_PRINT("read jpeg head failed: %d", ret);
    }

    jpeg_destroy_decompress(&cinfo);

    return JPEG_HEADER_OK;
}

static bool libjpeg_turbo_get_jpeg_direction(uint8_t * data, uint32_t data_size, uint32_t * orientation)
{
    struct jpeg_decompress_struct cinfo;
    error_mgr_t jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = libjpeg_turbo_error_exit;

    if(setjmp(jerr.jb)) {
        TY_GUI_LOG_PRINT("read jpeg orientation failed");
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, data, data_size);

    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xFFFF);

    cinfo.marker->read_markers(&cinfo);

    jpeg_saved_marker_ptr marker = cinfo.marker_list;
    while(marker != NULL) {
        if(marker->marker == JPEG_APP0 + 1) {
            JOCTET FAR * app1_data = marker->data;
            if(TRANS_32_VALUE(true, app1_data) == JPEG_EXIF) {
                uint16_t endian_tag = TRANS_16_VALUE(true, app1_data + 4 + 2);
                if(!(endian_tag == JPEG_LITTLE_ENDIAN_TAG || endian_tag == JPEG_BIG_ENDIAN_TAG)) {
                    jpeg_destroy_decompress(&cinfo);
                    return false;
                }
                bool is_big_endian = endian_tag == JPEG_BIG_ENDIAN_TAG;
                /* first ifd offset addr : 4bytes(Exif) + 2bytes(0x00) + 2bytes(align) + 2bytes(tag mark) */
                unsigned int offset = TRANS_32_VALUE(is_big_endian, app1_data + 8 + 2);
                /* ifd base : 4bytes(Exif) + 2bytes(0x00) */
                unsigned char * ifd = 0;
                do {
                    /* ifd start: 4bytes(Exif) + 2bytes(0x00) + offset value(2bytes(align) + 2bytes(tag mark) + 4bytes(offset size)) */
                    unsigned int entry_offset = 4 + 2 + offset + 2;
                    if(entry_offset >= marker->data_length) {
                        jpeg_destroy_decompress(&cinfo);
                        return false;
                    }
                    ifd = app1_data + entry_offset;
                    unsigned short num_entries = TRANS_16_VALUE(is_big_endian, ifd - 2);
                    if(entry_offset + num_entries * 12 >= marker->data_length) {
                        jpeg_destroy_decompress(&cinfo);
                        return false;
                    }
                    for(int i = 0; i < num_entries; i++) {
                        unsigned short tag = TRANS_16_VALUE(is_big_endian, ifd);
                        if(tag == 0x0112) {
                            /* ifd entry: 12bytes = 2bytes(tag number) + 2bytes(kind of data) + 4bytes(number of components) + 4bytes(data)
                            * orientation kind(0x03) of data is unsigned short */
                            int dirc = TRANS_16_VALUE(is_big_endian, ifd + 2 + 2 + 4);
                            switch(dirc) {
                                case 1:
                                    *orientation = 0;
                                    break;
                                case 3:
                                    *orientation = 180;
                                    break;
                                case 6:
                                    *orientation = 90;
                                    break;
                                case 8:
                                    *orientation = 270;
                                    break;
                                default:
                                    *orientation = 0;
                            }
                        }
                        ifd += 12;
                    }
                    offset = TRANS_32_VALUE(is_big_endian, ifd);
                } while(offset != 0);
            }
            break;
        }
        marker = marker->next;
    }

    jpeg_destroy_decompress(&cinfo);

    return JPEG_HEADER_OK;
}

static void libjpeg_turbo_error_exit(j_common_ptr cinfo)
{
    error_mgr_t * myerr = (error_mgr_t *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->jb, 1);
}
#endif /*TUYA_LIBJPEG_TURBO*/
