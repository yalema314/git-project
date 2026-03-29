#include "desk_event_handle.h"

chat_scr_res_t s_ai_res = {0}; 
static lv_style_t style_ai_message;
static lv_style_t style_user_message;

static lv_timer_t *chat_mode_timer = NULL;
static lv_obj_t *chat_mode_overlay = NULL; //全局覆盖层
static const char *chat_mode_en[] = {"LONG PRESS MODE", "ONESHOT PRESS MODE", "WAKEUP MODE", "FREE MODE", "P2P MODE", "TRANSLATE MODE", "PICTURE_GEN MODE"};
static const char *chat_mode_cn[] = {"长按模式", "按键模式", "唤醒模式", "自由模式", "P2P模式", "翻译模式", "生图模式"};

static void chat_styles_init(void) 
{
    // AI气泡样式
    lv_style_init(&style_ai_message);
    lv_style_set_bg_color(&style_ai_message, lv_color_white());
    lv_style_set_bg_opa(&style_ai_message, 0); //透明度
    lv_style_set_text_color(&style_ai_message, lv_color_white());
    lv_style_set_radius(&style_ai_message, 15);
    lv_style_set_pad_all(&style_ai_message, 12);
    lv_style_set_shadow_width(&style_ai_message, 0);
    lv_style_set_border_width(&style_ai_message, 0);

    // 用户气泡样式
    lv_style_init(&style_user_message);
    lv_style_set_bg_color(&style_user_message, lv_color_hex(0xB8BDDE));
    lv_style_set_bg_opa(&style_user_message, 28); //透明度
    lv_style_set_text_color(&style_user_message, lv_color_white());
    lv_style_set_radius(&style_user_message, 15);
    lv_style_set_pad_all(&style_user_message, 12);
    lv_style_set_shadow_width(&style_user_message, 0);
    lv_style_set_border_width(&style_user_message, 0);
}

lv_obj_t* create_chat_message(lv_obj_t **lable, bool is_ai) 
{
    lv_chat_ui_t *ui = &getContent()->st_chat;
    // 主消息容器
    lv_obj_t* msg_cont = lv_obj_create(ui->msg_container);
    lv_obj_remove_style_all(msg_cont);
    lv_obj_set_size(msg_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(msg_cont, 6, 0);
    lv_obj_set_flex_flow(msg_cont, is_ai ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_ROW_REVERSE);
    lv_obj_set_style_pad_column(msg_cont, 10, 0);

    /*---- 消息气泡 ----*/
    lv_obj_t* bubble = lv_obj_create(msg_cont);
    lv_obj_set_width(bubble, is_ai ? (LV_HOR_RES-40) : (LV_HOR_RES-85));
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);
    lv_obj_add_style(bubble, is_ai ? &style_ai_message : &style_user_message, 0);
    
    // 禁用所有滚动条和滑动
    lv_obj_set_scrollbar_mode(bubble, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(bubble, LV_DIR_NONE);
    lv_obj_clear_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);

    /*---- 消息内容 ----*/
    lv_obj_t* text_cont = lv_obj_create(bubble);
    lv_obj_remove_style_all(text_cont);
    lv_obj_set_size(text_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(text_cont, LV_FLEX_FLOW_COLUMN);
    // 禁用文本容器的滚动
    lv_obj_set_scrollbar_mode(text_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(text_cont, LV_DIR_NONE);

    // 消息文本
    *lable = lv_label_create(text_cont);
    // lv_obj_set_width(*lable, is_ai ? (LV_HOR_RES-40-10) : (LV_HOR_RES-85-10));
    lv_obj_set_width(*lable, is_ai ? (LV_HOR_RES-40-24) : (LV_HOR_RES-85-24));  // 减去padding
    lv_label_set_long_mode(*lable, LV_LABEL_LONG_WRAP);

    return msg_cont;    
}

void set_chat_message(uint8_t *data, bool is_ai) 
{
    lv_chat_ui_t *ui = &getContent()->st_chat;
    lv_obj_t *lable;
    lv_obj_t *parent = create_chat_message(&lable, is_ai);
    lv_label_set_text(lable, data);
    lv_obj_scroll_to_view(parent, LV_ANIM_ON);
    lv_obj_update_layout(ui->msg_container);    
}

void chat_message_test(lv_timer_t * timer)
{
    static int i = 0;
    set_chat_message("杭州今天天气怎么样？", false);
    set_chat_message("2025 年 12 月 29 日杭州市天气，多平台预报综合显示以阴或多云转晴为主，最低气温 3~6℃、最高气温 16~17℃，空气质量良至优（PM2.5 指数 48~74），风力多为 1~2 级，湿度 42%~92%，且未来 7 天（12 月 30 日 - 1 月 5 日）天气以多云、晴为主，1 月 1 日有小雨，气温整体呈波动下降趋势。", true);
    i++;

    if(i>=4)
    {
        lv_timer_del(timer);
    }
}

void setup_scr_chat_scr(void)
{
    TAL_PR_INFO("[%s] enter.", __func__);
    chat_styles_init();
    lv_chat_ui_t *ui = &getContent()->st_chat;

    ui->main_cont = lv_obj_create(NULL);
    lv_obj_set_size(ui->main_cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(ui->main_cont, lv_color_hex(0x25262A), 0);
    lv_obj_set_style_pad_all(ui->main_cont, 0, 0);

    if(AlibabaPuHuiTi3_55_18) {
        lv_obj_set_style_text_font(ui->main_cont, AlibabaPuHuiTi3_55_18, 0);
    } else {
        lv_obj_set_style_text_font(ui->main_cont, &AlibabaPuHuiTi3_Regular18, 0);
    }
    

    lv_obj_set_style_text_color(ui->main_cont, lv_color_white(), 0);
    lv_obj_set_scrollbar_mode(ui->main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(ui->main_cont, LV_DIR_NONE);

    if (png_img_load(tuya_app_gui_get_picture_full_path(ICON_AI_CHAT), &s_ai_res.ai_icon) == 0) 
    {
        ui->ai_icon = lv_img_create(ui->main_cont);
        lv_img_set_src(ui->ai_icon, &s_ai_res.ai_icon);
        lv_obj_set_pos(ui->ai_icon, 135, 10);
        lv_obj_set_size(ui->ai_icon, 48, 48);
    }
    

    // 消息容器
    ui->msg_container = lv_obj_create(ui->main_cont);
    lv_obj_set_size(ui->msg_container, LV_HOR_RES, LV_VER_RES - 58); 
    lv_obj_set_flex_flow(ui->msg_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(ui->msg_container, 0, 0);
    lv_obj_set_style_pad_ver(ui->msg_container, 8, 0);
    lv_obj_set_style_pad_hor(ui->msg_container, 10, 0);
    lv_obj_set_y(ui->msg_container, 58);

    lv_obj_move_background(ui->msg_container);

    // 禁用横向滚动
    lv_obj_set_scroll_dir(ui->msg_container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui->msg_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui->msg_container, LV_OPA_TRANSP, 0);

    lv_obj_update_layout(ui->main_cont);
    // lv_scr_load(ui->main_cont);

    lv_obj_add_event_cb(ui->main_cont, handle_chat_event, LV_EVENT_GESTURE, NULL);
    // lv_timer_t * timer = lv_timer_create(chat_message_test, 1000,  NULL);

    setDeskUIIndex(DESKUI_INDEX_CHAT);

    TAL_PR_INFO("[%s] quit.", __func__);
}

void chat_scr_res_clear(void)
{
    TAL_PR_INFO("[%s] enter.", __func__);

    //清除图片资源
    png_img_unload(&s_ai_res.ai_icon);

    //样式释放
    lv_style_reset(&style_ai_message);
    lv_style_reset(&style_user_message);

    //清除动态对象，set_chat_message创建的对象
    lv_chat_ui_t *ui = &getContent()->st_chat;
    if(ui->msg_container != NULL)
    {
        lv_obj_t *msg = lv_obj_get_child(ui->msg_container, NULL);
        while (msg != NULL) {
            lv_obj_t *next_msg = lv_obj_get_child(ui->msg_container, msg);
            lv_obj_del(msg); // 递归删除单个消息（包含气泡、文本等子对象）
            msg = next_msg;
            TAL_PR_INFO("[%s] set_chat_message obj clear.", __func__);
        }
        // lv_obj_del(ui->msg_container); // 删除消息容器本身
        // ui->msg_container = NULL;
        TAL_PR_INFO("[%s] set_chat_message obj clear finsh.", __func__);
    }

    memset(&s_ai_res, 0, sizeof(chat_scr_res_t));
}

void timer_ai_chat_mode_show(lv_timer_t * timer)
{
    TAL_PR_INFO("[%s] enter", __func__);

    if (chat_mode_overlay) 
    {
        lv_obj_del(chat_mode_overlay);
        chat_mode_overlay = NULL;
    }

    lv_obj_t *ui = lv_scr_act();
    if(ui)
    {
        TAL_PR_INFO("[%s] update scr act", __func__);
        lv_obj_update_layout(ui);
    }

    if(timer)
    {
        if (chat_mode_timer == timer) 
        {
            TAL_PR_INFO("[%s][%d] del chat mode timer", __func__, __LINE__);
            lv_timer_del(timer);
            chat_mode_timer = NULL;
        } 
        else 
        {
            TAL_PR_INFO("[%s][%d] del chat mode timer", __func__, __LINE__);
            lv_timer_del(timer);
        }
    }
} 

// 空回调，仅用来“消费”事件，阻止向下传递
static void overlay_event_cb(lv_event_t *e)
{
    (void)e;
}

void setup_scr_chat_mode(int mode)
{
    if (chat_mode_overlay) 
    {
        lv_obj_del(chat_mode_overlay);
        chat_mode_overlay = NULL;
    }

    if(chat_mode_timer)
    {
        lv_timer_del(chat_mode_timer);
        chat_mode_timer = NULL;
    }

    chat_mode_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(chat_mode_overlay);
    lv_obj_set_size(chat_mode_overlay, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(chat_mode_overlay, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(chat_mode_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(chat_mode_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_mode_overlay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    lv_obj_add_event_cb(chat_mode_overlay, overlay_event_cb, LV_EVENT_ALL, NULL);   //空事件绑定，防止透传事件

    
    lv_obj_t *chat_mode = lv_obj_create(chat_mode_overlay);
    lv_obj_remove_style_all(chat_mode);
    lv_obj_set_size(chat_mode, 320, 240);
    lv_obj_set_pos(chat_mode, 0, 0);
    lv_obj_set_style_bg_color(chat_mode, lv_color_hex(0x25262A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chat_mode, LV_OPA_80, 0);
    lv_obj_set_style_text_font(chat_mode, &AlibabaPuHuiTi3_Regular18, 0);
    lv_obj_set_scrollbar_mode(chat_mode, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(chat_mode, LV_DIR_NONE);
    lv_obj_set_flex_flow(chat_mode, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_mode, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    

    lv_obj_t *mode_label = lv_label_create(chat_mode);
    lv_obj_set_size(mode_label, LV_SIZE_CONTENT, 32);
    lv_label_set_long_mode(mode_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_border_width(mode_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(mode_label, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0xFFF37B), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(mode_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(mode_label, LV_OPA_30, 0);
    lv_obj_set_style_bg_color(mode_label, lv_color_hex(0xB8BDDE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(mode_label, 10, 0);
    lv_obj_set_style_pad_ver(mode_label, 7, 0);

    DESKTOP_LANGUAGE language_choice = getDeskLanguage();
    if(language_choice == DESK_CHINESE)
    {
        lv_label_set_text(mode_label, chat_mode_cn[mode]);
    }
    else if(language_choice == DESK_ENGLISH)
    {
        lv_label_set_text(mode_label, chat_mode_en[mode]);
    }

    chat_mode_timer = lv_timer_create(timer_ai_chat_mode_show, 3*1000, NULL);

    lv_obj_update_layout(chat_mode_overlay);

    setDeskUIIndex(DESKUI_INDEX_CHAT_MODE);
}