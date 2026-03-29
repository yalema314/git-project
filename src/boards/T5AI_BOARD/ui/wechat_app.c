#include "lvgl/lvgl.h"
#include "gui_common.h"
#include "tuya_ai_display.h"
#include "tal_log.h"

#define SCREEN_WIDTH  LV_HOR_RES
#define SCREEN_HEIGHT LV_VER_RES

/* 样式定义 */
static lv_style_t style_avatar;
static lv_style_t style_ai_bubble;
static lv_style_t style_user_bubble;
static lv_style_t style_time;

lv_obj_t* msg_container;
lv_obj_t* title;

static  lv_obj_t    *status_bar_ ;
static  lv_obj_t    *network_label_;
static  lv_obj_t    *status_label_;
static  lv_obj_t    *mode_label_;
static int __s_mode = 0;
static int __s_status = 0;
/* 计算动态气泡宽度 */
static inline uint32_t calc_bubble_width() {
    return SCREEN_WIDTH - 85; // 调整后更精确的宽度计算
}
LV_FONT_DECLARE(puhui_3bp_18);
LV_IMG_DECLARE(ai);
LV_IMG_DECLARE(user);
LV_FONT_DECLARE(font_awesome_20_4);

#define AI_MESSAGE_FONT    &puhui_3bp_18

static void SetStatus(uint8_t stat);

/* 创建消息元素 */
static lv_obj_t* create_message(lv_obj_t **lable, bool is_ai) 
{
    // 主消息容器
    lv_obj_t* msg_cont = lv_obj_create(msg_container);
    lv_obj_remove_style_all(msg_cont);
    lv_obj_set_size(msg_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(msg_cont, 6, 0);
    lv_obj_set_flex_flow(msg_cont, is_ai ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_ROW_REVERSE);
    lv_obj_set_style_pad_column(msg_cont, 10, 0);

    /*---- 头像 ----*/
    lv_obj_t* icon = lv_img_create(msg_cont);
    lv_obj_set_size(icon, 40, 40);
    lv_img_set_src(icon, is_ai ? &ai : &user);
    lv_obj_center(icon);

    /*---- 消息气泡 ----*/
    lv_obj_t* bubble = lv_obj_create(msg_cont);
    lv_obj_set_width(bubble, calc_bubble_width());
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);
    lv_obj_add_style(bubble, is_ai ? &style_ai_bubble : &style_user_bubble, 0);
    
    // 禁用所有滚动条
    lv_obj_set_scrollbar_mode(bubble, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(bubble, LV_DIR_NONE);

    /*---- 消息内容 ----*/
    lv_obj_t* text_cont = lv_obj_create(bubble);
    lv_obj_remove_style_all(text_cont);
    lv_obj_set_size(text_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(text_cont, LV_FLEX_FLOW_COLUMN);

    // 消息文本
    *lable = lv_label_create(text_cont);
    lv_obj_set_width(*lable, calc_bubble_width() - 24);
    lv_label_set_long_mode(*lable, LV_LABEL_LONG_WRAP);

    return msg_cont;
}

#define MAX_TEXT_LEN_IN_ONE_LABEL 255
static void SetDynamicMessage(TY_DISPLAY_TYPE_E type, uint8_t *data, int len) 
{
    static lv_obj_t *lable = NULL;
    static lv_obj_t *parent = NULL;
    static uint8_t buf[MAX_TEXT_LEN_IN_ONE_LABEL + 1] = {0};
    static uint8_t buf_len = 0;
    uint8_t *tmp_data = data;
    int tmp_len = len;
    BOOL_T need_new_label = FALSE;

    switch (type)
    {
    case TY_DISPLAY_TP_AI_CHAT_START:
        parent = create_message(&lable, true);
        memset(buf, 0, MAX_TEXT_LEN_IN_ONE_LABEL);
        buf_len = 0;
        // break;
    case TY_DISPLAY_TP_AI_CHAT_DATA:
        while (tmp_len > 0)
        {   
            if (need_new_label) {
                parent = create_message(&lable, true);
                memset(buf, 0, MAX_TEXT_LEN_IN_ONE_LABEL);
                buf_len = 0;
                need_new_label = FALSE;
            }

            if (buf_len < MAX_TEXT_LEN_IN_ONE_LABEL && tmp_len < MAX_TEXT_LEN_IN_ONE_LABEL - buf_len) {
                // have enough space for new data
                strncat(buf, tmp_data, strlen(tmp_data));
                buf_len += strlen(tmp_data);

                TAL_PR_DEBUG("1-buf %s, buf len %d, data %s len %d, strlen %d", buf, buf_len, tmp_data, tmp_len, strlen(buf));
                lv_label_set_text(lable, buf);
                lv_obj_scroll_to_view(parent, LV_ANIM_ON);
                lv_obj_update_layout(msg_container);
                break;
            } else if (buf_len < MAX_TEXT_LEN_IN_ONE_LABEL && len > MAX_TEXT_LEN_IN_ONE_LABEL - buf_len) {
                // no enough space for new data
                need_new_label = TRUE;
            } else {
                TAL_PR_DEBUG("buf %s, buf len %d, data %s len %d", buf, buf_len, data, len);
                break;
            }
        }
        break;    
    default:
        break;
    }    
}

void SetStaticMessage(uint8_t *data, bool is_ai) 
{
    lv_obj_t *lable;
    lv_obj_t *parent = create_message(&lable, is_ai);
    lv_label_set_text(lable, data);
    lv_obj_scroll_to_view(parent, LV_ANIM_ON);
    lv_obj_update_layout(msg_container);
}


/* 初始化样式 */
static void init_styles(void) 
{
    // 头像样式
    lv_style_init(&style_avatar);
    lv_style_set_radius(&style_avatar, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&style_avatar, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_border_width(&style_avatar, 1);
    lv_style_set_border_color(&style_avatar, lv_palette_darken(LV_PALETTE_GREY, 2));

    // AI气泡样式
    lv_style_init(&style_ai_bubble);
    lv_style_set_bg_color(&style_ai_bubble, lv_color_white());
    lv_style_set_radius(&style_ai_bubble, 15);
    lv_style_set_pad_all(&style_ai_bubble, 12);
    lv_style_set_shadow_width(&style_ai_bubble, 12);
    lv_style_set_shadow_color(&style_ai_bubble, lv_color_hex(0xCCCCCC));

    // 用户气泡样式
    lv_style_init(&style_user_bubble);
    lv_style_set_bg_color(&style_user_bubble, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_text_color(&style_user_bubble, lv_color_white());
    lv_style_set_radius(&style_user_bubble, 15);
    lv_style_set_pad_all(&style_user_bubble, 12);
    lv_style_set_shadow_width(&style_user_bubble, 12);
    lv_style_set_shadow_color(&style_user_bubble, lv_palette_darken(LV_PALETTE_GREEN, 2));
}


static void create_ai_chat_ui(void) 
{
    init_styles();

    // 主容器
    lv_obj_t* main_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_cont, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    // lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_text_font(main_cont, AI_MESSAGE_FONT, 0);
    lv_obj_set_style_text_color(main_cont, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(main_cont, LV_DIR_NONE);

    // 标题栏
    status_bar_ = lv_obj_create(main_cont);
    lv_obj_set_size(status_bar_, LV_HOR_RES, 40);
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(status_bar_, 0, 0);
    lv_obj_set_style_border_width(status_bar_, 0, 0);
    lv_obj_set_style_pad_column(status_bar_, 0, 0);
    lv_obj_set_style_pad_left(status_bar_, 5, 0);
    lv_obj_set_style_pad_right(status_bar_, 5, 0);
    lv_obj_set_flex_align( status_bar_, LV_FLEX_ALIGN_CENTER,  LV_FLEX_ALIGN_CENTER,  LV_FLEX_ALIGN_CENTER);

    mode_label_ = lv_label_create(status_bar_);
    lv_obj_set_style_text_align(mode_label_, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(mode_label_, gui_mode_desc_get(__s_mode));

    // status = 0;
    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(status_label_, 1);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    SetStatus(__s_status);

    network_label_ = lv_label_create(status_bar_);
    lv_obj_set_style_text_font(network_label_, &font_awesome_20_4, 0);
    lv_label_set_text(network_label_, FONT_AWESOME_WIFI_OFF);

    // 消息容器
    msg_container = lv_obj_create(main_cont);
    lv_obj_set_size(msg_container, SCREEN_WIDTH, SCREEN_HEIGHT - 40); 
    lv_obj_set_flex_flow(msg_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(msg_container, 0, 0);
    lv_obj_set_style_pad_ver(msg_container, 8, 0);
    lv_obj_set_style_pad_hor(msg_container, 10, 0);
    lv_obj_set_y(msg_container, 40);

    lv_obj_move_background(msg_container);

    // 禁用横向滚动
    lv_obj_set_scroll_dir(msg_container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(msg_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(msg_container, LV_OPA_TRANSP, 0);
}

void tuya_wechat_init(void)
{
    create_ai_chat_ui();
    SetStaticMessage("hi, 你好啊，我是涂鸭鸭", true);
}

static void SetStatus(uint8_t stat) 
{
    if (status_label_ == NULL) {
        return;
    }

    char *text;

    if (OPRT_OK != gui_status_desc_get(stat, &text, NULL)) {
        return;
    }

    TAL_PR_DEBUG("status is %s", text);
    lv_label_set_text(status_label_, text);
}


void wechat_text_disp_cb(void *obj, char *text, int pos, void *priv_data)
{
    if (!obj || !priv_data) {
        return;
    }

    if (pos) {
        lv_label_ins_text(obj, pos, text);
    } else {
        lv_label_set_text(obj, text);
    }

    // 确保滚动到底部
    lv_obj_scroll_to_view(priv_data, LV_ANIM_ON);
    lv_obj_update_layout(msg_container);
}

static const char *power_txet  = "你好啊，我来了，让我们一起玩耍吧";
static const char *netok_txet  = "我已联网，让我们开始对话吧";
static const char *netcfg_txet = "我已进入配网状态, 你能帮我用涂鸦智能app配网嘛";
#define MAX_LEN_DISPLAY_ONE_TIME 4096
void tuya_wechat_app(TY_DISPLAY_MSG_T *msg)
{
    switch (msg->type) {

    case TY_DISPLAY_TP_LANGUAGE:
        gui_lang_set(msg->data[0]);
        SetStatus(__s_status);
        lv_label_set_text(mode_label_, gui_mode_desc_get(__s_mode));
        // gui_txet_disp_init(NULL, NULL, 0, 0);
        // gui_txet_disp_set_cb(wechat_text_disp_cb);
        break;

    case TY_DISPLAY_TP_HUMAN_CHAT:
        SetStaticMessage(msg->data, FALSE);
        // gui_text_disp_free();
        break;

    case TY_DISPLAY_TP_AI_CHAT_START: 
    case TY_DISPLAY_TP_AI_CHAT_DATA: 
    case TY_DISPLAY_TP_AI_CHAT_STOP: 
        SetDynamicMessage(msg->type, msg->data, msg->len);
        break;

    case TY_DISPLAY_TP_STAT_POWERON:
        SetStaticMessage(power_txet, TRUE);
        break;

    case TY_DISPLAY_TP_STAT_ONLINE:
        SetStatus(GUI_STAT_IDLE);
        SetStaticMessage(netok_txet, TRUE);
        break;

    case TY_DISPLAY_TP_CHAT_STAT:
        // if (GUI_STAT_IDLE == msg->data[0]) {
        //     gui_text_disp_stop();
        // } else if (GUI_STAT_LISTEN == msg->data[0]) {
        //     gui_text_disp_stop();
        // } else if (GUI_STAT_SPEAK == msg->data[0]) {
        //     lv_obj_t *lable, *parent;
        //     parent = create_message(&lable, true);
        //     gui_text_disp_set_windows(lable, parent, 0, 0);
        //     gui_text_disp_start();
        // }
        __s_status = msg->data[0];
        SetStatus(msg->data[0]);
        break;

    case TY_DISPLAY_TP_STAT_SLEEP:
        SetStatus(GUI_STAT_IDLE);
        break;

    case TY_DISPLAY_TP_STAT_NET:
        lv_label_set_text(network_label_, gui_wifi_level_get(msg->data[0]));
        break;

    case TY_DISPLAY_TP_CHAT_MODE: 
        __s_mode = msg->data[0];
        lv_label_set_text(mode_label_, gui_mode_desc_get(__s_mode)); 
        SetStatus(GUI_STAT_IDLE);
        break;

    case TY_DISPLAY_TP_STAT_NETCFG:
        SetStatus(GUI_STAT_PROV);
        SetStaticMessage(netcfg_txet, TRUE);
        break;

    }
}
