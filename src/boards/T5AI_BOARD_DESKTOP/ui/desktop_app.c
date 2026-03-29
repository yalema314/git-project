#include "lvgl/lvgl.h"
#include "gui_common.h"
#include "tuya_ai_display.h"
#include "tal_log.h"
#include "desk_event_handle.h"
static bool is_first_start = true;

extern void desktop_ui_startup(void);

#if 1

VOID tuya_desktop_init(VOID)
{
#if defined(TUYA_FILE_SYSTEM) && (TUYA_FILE_SYSTEM == 1)
    TAL_PR_INFO("[%s] enter", __FUNCTION__);
	desktop_ui_startup();
#endif
}

void tuya_desktop_app(TY_DISPLAY_MSG_T *msg)
{
#if defined(TUYA_FILE_SYSTEM) && (TUYA_FILE_SYSTEM == 1)

    TAL_PR_DEBUG("[%s] type: %d", __func__, msg->type);

    switch(msg->type)
    {
        case TY_DISPLAY_TP_HUMAN_CHAT:
        case TY_DISPLAY_TP_AI_CHAT_START: 
        case TY_DISPLAY_TP_AI_CHAT_DATA: 
        case TY_DISPLAY_TP_AI_CHAT_STOP:
        {
            receive_ai_message_data(msg->type, msg->data, msg->len);
        }
        break;

        case TY_DISPLAY_TP_EMOJI:
        {
            TAL_PR_INFO("[%s] emotion: %s", __func__, msg->data);
            receive_emotional_feedback(msg->data, msg->len);
        }
        break;

        case TY_DISPLAY_TP_CHAT_MODE: 
        {
            if(is_first_start)
            {
                is_first_start = false;
                break;
            }
            receive_ai_chat_mode_data(msg->data, msg->len);
        }
        break;

        default:
        break;
    }
    
#endif
}

#else

lv_obj_t *ui = NULL;
lv_obj_t *left = NULL;
lv_obj_t *stop = NULL;
lv_obj_t *right = NULL;

lv_obj_t *chat_mode = NULL;


void timer_switch_mode_test(lv_timer_t * timer)
{
    TAL_PR_DEBUG("timer_switch_mode_test!!!!!!!!!!");

    lv_obj_t *ui = lv_scr_act();
    if(ui == NULL)
    {
        TAL_PR_DEBUG("[%s] ui is null", __func__);
        return;
    }

    if(chat_mode)
    {
        TAL_PR_DEBUG("timer_switch_mode_test del chat_mode obj");
        lv_obj_del(chat_mode);
        chat_mode = NULL;
    }

    lv_obj_update_layout(ui);

    if(timer)
    {
        TAL_PR_DEBUG("timer_switch_mode_test del switch chat mode timer");
        lv_timer_del(timer);
    }   
}

void switch_chat_mode_test(void)
{
    lv_obj_t *ui = lv_scr_act();
    if(ui == NULL)
    {
        TAL_PR_DEBUG("[%s] ui is null", __func__);
        return;
    }

    chat_mode = lv_obj_create(ui);
    lv_obj_remove_style_all(chat_mode);
    lv_obj_set_size(chat_mode, 240, 100);
    lv_obj_set_pos(chat_mode, 40, 70);
    lv_obj_set_style_bg_opa(chat_mode, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_font(chat_mode, &AlibabaPuHuiTi3_Regular18, 0);
    lv_obj_set_scrollbar_mode(chat_mode, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(chat_mode, LV_DIR_NONE);

    lv_obj_set_flex_flow(chat_mode, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_mode, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    

    lv_obj_t *mode_label = lv_label_create(chat_mode);
    lv_label_set_text(mode_label, "LONG PRESS MODE");
    // lv_obj_set_pos(mode_label, 6, 34);
    // lv_obj_set_size(mode_label, 188, 32);
    lv_obj_set_size(mode_label, LV_SIZE_CONTENT, 32);

    lv_label_set_long_mode(mode_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_border_width(mode_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(mode_label, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0xFFF37B), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(mode_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(mode_label, 28, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(mode_label, lv_color_hex(0xB8BDDE), LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_pad_top(mode_label, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(mode_label, 10, 0);
    lv_obj_set_style_pad_ver(mode_label, 7, 0);


    lv_timer_t *timer = lv_timer_create(timer_switch_mode_test, 5*1000, NULL);

    lv_obj_update_layout(ui);


}


extern OPERATE_RET tuya_motion_send_msg(UINT_T mode, UINT_T rotate_value);

void motion_test_btn(lv_event_t *e)
{
    
    lv_event_code_t code = lv_event_get_code(e);
    TAL_PR_DEBUG("[motion_test_btn] ====================enter[%d]==================== ", code);

    if (code == LV_EVENT_CLICKED) 
    {
        lv_obj_t *target = lv_event_get_target(e);  // 获取触发事件的对象
        if (target == left) 
        {
            TAL_PR_DEBUG("[motion_test_btn] ====================left==================== ");
            tuya_motion_send_msg(0, 360);

        }
        else if (target == stop) 
        {
            TAL_PR_DEBUG("[motion_test_btn] ====================stop==================== ");
            tuya_motion_send_msg(7, 0);

        }
        else if (target == right) 
        {
            TAL_PR_DEBUG("[motion_test_btn] ====================right==================== ");
            tuya_motion_send_msg(1, 360);

        }
        else
        {
            TAL_PR_DEBUG("[motion_test_btn] ====================null[%p]==================== ", target);
            TAL_PR_DEBUG("[motion_test_btn] ====================left[%p]==================== ", left);
            TAL_PR_DEBUG("[motion_test_btn] ====================stop[%p]==================== ", stop);
            TAL_PR_DEBUG("[motion_test_btn] ====================right[%p]==================== ", right);
        }

    }    
}

VOID tuya_desktop_init(VOID)
{
    lv_obj_t *ui = lv_obj_create(NULL);
    lv_obj_set_size(ui, 320, 240);
    lv_obj_set_scrollbar_mode(ui, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui, lv_color_hex(0x25262A), LV_PART_MAIN|LV_STATE_DEFAULT);

    // left = lv_btn_create(ui);
    // lv_obj_set_pos(left, 0, 95);
    // lv_obj_set_size(left, 100, 50);

    // stop = lv_btn_create(ui);
    // lv_obj_set_pos(stop, 110, 95);
    // lv_obj_set_size(stop, 100, 50);

    // right = lv_btn_create(ui);
    // lv_obj_set_pos(right, 220, 95);
    // lv_obj_set_size(right, 100, 50);

    //Update current screen layout.
    lv_obj_update_layout(ui);

    // lv_obj_add_event_cb(left, motion_test_btn, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_event_cb(stop, motion_test_btn, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_event_cb(right, motion_test_btn, LV_EVENT_CLICKED, NULL);

    lv_scr_load(ui);
}

void tuya_desktop_app(TY_DISPLAY_MSG_T *msg)
{
    TAL_PR_DEBUG("[%s] type: %d", __func__, msg->type);

    switch(msg->type)
    {
        case TY_DISPLAY_TP_CHAT_MODE: 
        {
            switch_chat_mode_test();
        }
        break;

        default:
        break;
    }
}

#endif

