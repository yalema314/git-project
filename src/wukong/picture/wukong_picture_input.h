/**
 * @file wukong_picture_input.h
 * @brief Wukong picture input interface definitions
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

#ifndef __WUKONG_PICTURE_INPUT_H__
#define __WUKONG_PICTURE_INPUT_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Send picture data to the AI agent
 * 
 * This function sends picture data to the AI agent for processing.
 * The picture data will be forwarded to the cloud AI service for analysis.
 * 
 * @param[in] data Pointer to picture data buffer
 * @param[in] len Length of picture data in bytes
 * 
 * @return OPRT_OK on success, error code otherwise
 */
OPERATE_RET wukong_picture_input(UINT8_T *data, UINT_T len);

#ifdef __cplusplus
}
#endif

#endif /* __WUKONG_PICTURE_INPUT_H__ */
