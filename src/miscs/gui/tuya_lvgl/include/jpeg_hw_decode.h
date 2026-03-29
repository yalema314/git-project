#ifndef __JPEG_HW_DECODE_H_
#define __JPEG_HW_DECODE_H_
#include "tuya_cloud_types.h"

//#define ENABLE_JPEG_IMAGE_DECODE_WITH_HW

OPERATE_RET jpeg_hw_decode(void *_jpeg_frame, void *_img_dst);
#endif

