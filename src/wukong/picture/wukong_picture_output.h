/**
 * @file wukong_picture_output.h
 * @brief Wukong picture output interface definitions
 * @version 1.0.0
 * @date 2025-01-01
 *
 * @copyright Copyright (c) 2025 Tuya Inc. All Rights Reserved.
 *
 * Permission is hereby granted, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), Under the premise of complying
 * with the license of the third-party open source software contained in the software,
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software.
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 */

#ifndef __WUKONG_PICTURE_OUTPUT_H__
#define __WUKONG_PICTURE_OUTPUT_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Picture output callback function type
 * 
 * This callback function is called when picture data needs to be output.
 * The callback should handle the picture data appropriately (e.g., display on screen,
 * save to file, send to network, etc.).
 * 
 * @param[in] data Pointer to picture data buffer
 * @param[in] len Length of picture data in bytes
 * @param[in] is_eif Indicate the picture data is finished
 * 
 * @return OPRT_OK on success, error code otherwise
 */
typedef INT_T (*WUKONG_PICTURE_CB)(UINT8_T *data, INT_T len, BOOL_T is_eof);

/**
 * @brief Initialize the picture output module
 * 
 * This function initializes the picture output module and registers
 * the callback function for picture data output.
 * 
 * @param[in] cb Picture output callback function to be called when picture data is ready
 * 
 * @return OPRT_OK on success, error code otherwise
 */
OPERATE_RET wukong_picture_output_init(WUKONG_PICTURE_CB cb);

/**
 * @brief Deinitialize the picture output module
 * 
 * This function deinitializes the picture output module and releases
 * all associated resources.
 * 
 * @return OPRT_OK on success, error code otherwise
 */
OPERATE_RET wukong_picture_output_deinit(VOID);

/**
 * @brief Output picture data through registered callback
 * 
 * This function is called when picture data needs to be output.
 * It invokes the registered callback function if available.
 * 
 * @param[in] url The url of the picture for ai
 * 
 * @return OPRT_OK on success, OPRT_RESOURCE_NOT_READY if callback not registered, error code otherwise
 */
OPERATE_RET wukong_picture_output_process(CHAR_T *url);

#ifdef __cplusplus
}
#endif

#endif /* __WUKONG_PICTURE_OUTPUT_H__ */
