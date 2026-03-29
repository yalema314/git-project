/**
 * @file wukong_picture_output.c
 * @brief Wukong picture output implementation - handles picture data output through callback
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

#include "wukong_picture_output.h"
#include "uni_log.h"

/**
 * @brief Static global picture output callback function pointer
 * 
 * This pointer holds the registered callback function for picture data output.
 */
STATIC WUKONG_PICTURE_CB __s_picture_output_cb = NULL;

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
OPERATE_RET wukong_picture_output_init(WUKONG_PICTURE_CB cb)
{
    if (cb == NULL) {
        TAL_PR_ERR("wukong picture output -> invalid callback function");
        return OPRT_INVALID_PARM;
    }

    __s_picture_output_cb = cb;
    TAL_PR_DEBUG("wukong picture output -> initialized");
    
    return OPRT_OK;
}

/**
 * @brief Deinitialize the picture output module
 * 
 * This function deinitializes the picture output module and releases
 * all associated resources. It clears the callback function pointer.
 * 
 * @return OPRT_OK on success, error code otherwise
 */
OPERATE_RET wukong_picture_output_deinit(VOID)
{
    __s_picture_output_cb = NULL;
    TAL_PR_DEBUG("wukong picture output -> deinitialized");
    
    return OPRT_OK;
}

/**
 * @brief Output picture data through registered callback
 * 
 * This function is called when picture data needs to be output.
 * It invokes the registered callback function if available.
 * 
 * @param[in] url The url of the picture
 * 
 * @return OPRT_OK on success, OPRT_RESOURCE_NOT_READY if callback not registered, error code otherwise
 */
OPERATE_RET wukong_picture_output_process(CHAR_T *url)
{
    if (__s_picture_output_cb == NULL) {
        TAL_PR_ERR("wukong picture output -> callback not registered, ignore");
        return OPRT_RESOURCE_NOT_READY;
    }

    if (url == NULL) {
        TAL_PR_ERR("wukong picture output -> invalid parameters");
        return OPRT_INVALID_PARM;
    }

    // TODO
    // run a thread to download the picture and send to __s_picture_output_cb

    return OPRT_OK;
    // return __s_picture_output_cb(data, len);
}
