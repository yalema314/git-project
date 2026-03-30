/**
 * @file tuya_device_board.c
 * @author www.tuya.com
 * @brief tuya_device_board module is used to
 * @version 0.1
 * @date 2022-10-28
 *
 * @copyright Copyright (c) tuya.inc 2022
 *
 */
#include "tuya_device_board.h"
#include "tuya_cloud_types.h"
#include "app_gesture.h"
#include "tuya_robot_actions.h"
#include "tuya_ai_display.h"

#if defined(TUYA_AI_TOY_BATTERY_ENABLE) && (TUYA_AI_TOY_BATTERY_ENABLE == 1)
#include "tuya_ai_battery.h"
#endif

/***********************************************************
************************macro define************************
***********************************************************/

OPERATE_RET __emoji_show(uint8_t dir)
{
    STATIC CONST CHAR_T *emotion[] = {
        "neutral",
        "annoyed",
        "cool",
        "delicious",
        "fearful",
        "lovestruck",
        "loving",
        "unamused",
        "winking",
        "zany",
        /*----------------- */
        "crying",
        "angry",
        "confused",
        "disappointed",
        "embarrassed",
        "happy",
        "laughing",
        "relaxed",
        "sad",
        "surprise",
        "thinking",
    };
    STATIC UINT8_T index = 0;
    UINT8_T num = sizeof(emotion) / sizeof(emotion[0]);


    if (dir) {
        index = (index + 1) % num;
    } else {
        index = (0 == index) ? num - 1 : index - 1;
    }

    if (index > num) {
        return OPRT_NOT_EXIST;
    }

    return tuya_ai_display_msg(emotion[index], strlen(emotion[index]), TY_DISPLAY_TP_EMOJI);
}

 VOID ai_robot_gesture_cb(GESTURE_TYPE_E gesture)
 {
    //TAL_PR_NOTICE("ai_toy_gesture_cb gesture %d", gesture);
    if (gesture == GESTURE_LEFT) {
        __emoji_show(0);
    } else if (gesture == GESTURE_RIGHT) {
        __emoji_show(1);
    }
} 

/**
 * @brief evb board initialization
 *
 * @param[in] none
 *
 * @return OPRT_OK on success. Others on error, please refer to "tuya_error_code.h".
 */
OPERATE_RET tuya_device_board_init(VOID_T)
{
    OPERATE_RET rt = OPRT_OK;

    TAL_PR_NOTICE("ai toy -> init action");
    TUYA_CALL_ERR_LOG(tuya_robot_action_init());
    TUYA_CALL_ERR_LOG(app_gesture_init(ai_robot_gesture_cb));
#if defined(TUYA_AI_TOY_BATTERY_ENABLE) && (TUYA_AI_TOY_BATTERY_ENABLE == 1)
    tuya_ai_toy_charge_level_set(TUYA_GPIO_LEVEL_HIGH);
#endif

    return rt;
}