#ifndef __TUYA_AI_TOY_MCP_H__
#define __TUYA_AI_TOY_MCP_H__

#include "wukong_ai_mcp_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the MCP server for the AI toy
 *
 * @return OPERATE_RET OPRT_OK on success, error code otherwise
 */
OPERATE_RET tuya_ai_toy_mcp_init(VOID);

/**
 * @brief Deinitialize the MCP server for the AI toy
 *
 * @return OPERATE_RET OPRT_OK on success, error code otherwise
 */
OPERATE_RET tuya_ai_toy_mcp_deinit(VOID);

#ifdef __cplusplus
}
#endif

#endif