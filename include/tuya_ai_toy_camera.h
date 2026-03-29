/**
 * @file tuya_ai_toy_camera.h
 * @brief Camera module for Tuya AI toy: init by type (local/LAN/P2P), start/stop, and JPEG frame capture.
 */

#ifndef __TUYA_AI_TOY_CAMERA_H__
#define __TUYA_AI_TOY_CAMERA_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Camera usage type. */
typedef enum {
    AI_TOY_CAMERA_LOCAL,  /**< Take a photo (local capture). */
    AI_TOY_CAMERA_LAN,    /**< Stream to LAN (e.g. RTSP server). */
    AI_TOY_CAMERA_P2P,    /**< Stream to remote (P2P). */
} AI_TOY_CAMERA_TYPE_E;

/**
 * @brief Initialize the AI toy camera for the given type.
 * @param[in] type One of AI_TOY_CAMERA_LOCAL, AI_TOY_CAMERA_LAN, AI_TOY_CAMERA_P2P.
 * @return OPRT_OK on success. See tuya_error_code.h for errors.
 */
OPERATE_RET tuya_ai_toy_camera_init(AI_TOY_CAMERA_TYPE_E type);

/**
 * @brief De-initialize the camera and release resources.
 * @return OPRT_OK on success. See tuya_error_code.h for errors.
 */
OPERATE_RET tuya_ai_toy_camera_deinit(VOID);

/**
 * @brief Start the camera (stream or capture pipeline).
 * @return OPRT_OK on success. See tuya_error_code.h for errors.
 */
OPERATE_RET tuya_ai_toy_camera_start(VOID);

/**
 * @brief Get one JPEG frame from the camera.
 * @param[out] data     Pointer to receive the frame buffer (allocated by implementation; caller must free).
 * @param[out] len      Pointer to receive the frame length in bytes.
 * @param[in]  user_data Optional user context (implementation-specific).
 * @return OPRT_OK on success. See tuya_error_code.h for errors.
 */
OPERATE_RET tuya_ai_toy_camera_get_jpeg_frame(BYTE_T **data, UINT_T *len, VOID *user_data);

/**
 * @brief Stop the camera.
 * @return OPRT_OK on success. See tuya_error_code.h for errors.
 */
OPERATE_RET tuya_ai_toy_camera_stop(VOID);

#ifdef __cplusplus
}
#endif

#endif /* __TUYA_AI_TOY_CAMERA_H__ */
