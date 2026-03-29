#include "desk_event_handle.h"

netcfg_scr_res_t s_netcfg_scr_res = {0};

static void handle_startup_welcome_timer(lv_timer_t * timer)
{
    TAL_PR_INFO("[%s] enter", __func__);

    lv_startup_ui_t *startup_ui = &getContent()->st_startup;
    lv_home_ui_t *home_ui = &getContent()->st_home;


    if(getContent()->active_stat >= ACTIVATED)  //设备已激活.直接进入home界面
    {
        TAL_PR_DEBUG("[%s] quit welcome screen, load home screen", __func__);
        switch_ui_scr_animation(&home_ui->home1_lv.home_scr1, setup_scr_home_scr1, LV_SCR_LOAD_ANIM_NONE, SWITCH_SCREEN_PERMANENT);
    }
    else    //设备未激活，走配网流程
    {
        TAL_PR_DEBUG("[%s] quit welcome screen, load language screen", __func__);
        switch_ui_scr_animation(&startup_ui->language_lv.language_scr, setup_scr_language_scr, LV_SCR_LOAD_ANIM_NONE, SWITCH_SCREEN_PERMANENT);
    }
    
    lv_timer_del(timer);
}

void setup_scr_startup(void)
{
    TAL_PR_INFO("[%s] enter", __func__);
    lv_startup_ui_t *ui = &getContent()->st_startup;
    ui->startup_scr = lv_obj_create(NULL);
    lv_obj_set_size(ui->startup_scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(ui->startup_scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->startup_scr, lv_color_hex(0x25262A), 0);
    lv_obj_set_style_pad_all(ui->startup_scr, 0, 0);

    lv_obj_t *welcome_text = lv_label_create(ui->startup_scr);
    lv_label_set_text(welcome_text, "Welcome");
    lv_obj_set_size(welcome_text, 132, 32);
    lv_obj_set_pos(welcome_text, 94, 99);
    lv_obj_set_style_text_font(welcome_text, &AlibabaPuHuiTi3_Regular30, 0);
    lv_obj_set_style_text_color(welcome_text, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(welcome_text, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_update_layout(ui->startup_scr);

    lv_timer_t * timer = lv_timer_create(handle_startup_welcome_timer, 1000,  NULL);

    setDeskUIIndex(DESKUI_INDEX_STARTUP);
}

static void handle_chinese_btn_clicked(lv_event_t *e)
{
    lv_startup_ui_t *ui = &getContent()->st_startup;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("Chinese selected");
        
        setDeskLanguage(DESK_CHINESE);

        lv_indev_wait_release(lv_indev_get_act());
        
        switch_ui_scr_animation(&ui->qrcode_lv.qrcode_scr, setup_scr_qrcode_scr, LV_SCR_LOAD_ANIM_NONE, SWITCH_SCREEN_PERMANENT);
        
    }    
}

static void handle_english_btn_clicked(lv_event_t *e)
{
    lv_startup_ui_t *ui = &getContent()->st_startup;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("English selected");
        
        setDeskLanguage(DESK_ENGLISH);

        lv_indev_wait_release(lv_indev_get_act());
        
        switch_ui_scr_animation(&ui->qrcode_lv.qrcode_scr, setup_scr_qrcode_scr, LV_SCR_LOAD_ANIM_NONE, SWITCH_SCREEN_PERMANENT);
        
    }    
}

void setup_scr_language_scr(void)
{
    TAL_PR_INFO("[%s] enter", __func__);
    language_scr_t *ui = &getContent()->st_startup.language_lv;
    ui->language_scr = lv_obj_create(NULL);
    lv_obj_set_size(ui->language_scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(ui->language_scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->language_scr, lv_color_hex(0x25262A), 0);
    lv_obj_set_style_pad_all(ui->language_scr, 0, 0);

    lv_obj_t *chinese_btn = lv_btn_create(ui->language_scr);
    lv_obj_remove_style_all(chinese_btn);
    lv_obj_set_size(chinese_btn, 240, 54);
    lv_obj_set_pos(chinese_btn, 40, 60);
    lv_obj_set_style_bg_opa(chinese_btn, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(chinese_btn, lv_color_hex(0xFFDC7B), 0);
    lv_obj_set_style_radius(chinese_btn, 27, 0);
    lv_obj_add_event_cb(chinese_btn, handle_chinese_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *chinese_text = lv_label_create(chinese_btn);
    lv_label_set_text(chinese_text, "中文");
    lv_obj_set_size(chinese_text, LV_SIZE_CONTENT, 30);
    lv_obj_align(chinese_text, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_font(chinese_text, &AlibabaPuHuiTi3_Regular30, 0);
    lv_obj_set_style_text_color(chinese_text, lv_color_hex(0xFFF37B), 0);
    lv_obj_set_style_text_align(chinese_text, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中
    
    lv_obj_t *english_btn = lv_btn_create(ui->language_scr);
    lv_obj_remove_style_all(english_btn);
    lv_obj_set_size(english_btn, 240, 54);
    lv_obj_set_pos(english_btn, 40, 125);
    lv_obj_set_style_bg_opa(english_btn, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(english_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(english_btn, 27, 0);
    lv_obj_add_event_cb(english_btn, handle_english_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *english_text = lv_label_create(english_btn);
    lv_label_set_text(english_text, "English");
    lv_obj_set_size(english_text, LV_SIZE_CONTENT, 30);
    lv_obj_align(english_text, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_font(english_text, &AlibabaPuHuiTi3_Regular30, 0);
    lv_obj_set_style_text_color(english_text, lv_color_white(), 0);
    lv_obj_set_style_text_align(english_text, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_update_layout(ui->language_scr);

}

int event_netcfg_start(void *data)
{
    TAL_PR_INFO("[%s] enter", __func__);
    network_start_scr_t *ui = &getContent()->st_startup.network_start_lv;
    switch_ui_scr_animation(&ui->cfg_network_scr, cfg_network_start, LV_SCR_LOAD_ANIM_NONE, SWITCH_SCREEN_PERMANENT);
}

int event_netcf_success(void *data)
{
    TAL_PR_INFO("[%s] enter", __func__);
    cfg_network_success();
}

int event_netcf_fail(void *data)
{
    TAL_PR_INFO("[%s] enter", __func__);
}

void setup_scr_qrcode_scr(void)
{
    TAL_PR_INFO("[%s] enter", __func__);
    qrcode_scr_t *ui = &getContent()->st_startup.qrcode_lv;
    ui->qrcode_scr = lv_obj_create(NULL);
    lv_obj_set_size(ui->qrcode_scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(ui->qrcode_scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->qrcode_scr, lv_color_hex(0x25262A), 0);
    lv_obj_set_style_pad_all(ui->qrcode_scr, 0, 0);

    lv_obj_t *qrcode = lv_qrcode_create(ui->qrcode_scr, 90, lv_color_white(), lv_color_black());
    lv_qrcode_update(qrcode, "https://www.tuya.com/cn/", 24);
    lv_obj_set_pos(qrcode, 115, 42);
    lv_obj_set_size(qrcode, 90, 90);

    lv_obj_t *keyword_text_1 = lv_label_create(ui->qrcode_scr);
    lv_label_set_text(keyword_text_1, "扫描二维码");
    lv_obj_set_size(keyword_text_1, LV_SIZE_CONTENT, 20);
    lv_obj_set_pos(keyword_text_1, 110, 148);
    lv_obj_set_style_text_font(keyword_text_1, &AlibabaPuHuiTi3_Regular20, 0);
    lv_obj_set_style_text_color(keyword_text_1, lv_color_white(), 0);
    lv_obj_set_style_text_align(keyword_text_1, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_t *keyword_text_2 = lv_label_create(ui->qrcode_scr);
    lv_label_set_text(keyword_text_2, "下载APP添加设备");
    lv_obj_set_size(keyword_text_2, LV_SIZE_CONTENT, 18);
    lv_obj_set_pos(keyword_text_2, 88, 178);
    lv_obj_set_style_text_font(keyword_text_2, &AlibabaPuHuiTi3_Regular18, 0);
    lv_obj_set_style_text_color(keyword_text_2, lv_color_white(), 0);
    lv_obj_set_style_text_align(keyword_text_2, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_update_layout(ui->qrcode_scr);

    ty_subscribe_event(EVENT_NETCFG_DATA, "ai_desktop", event_netcfg_start, SUBSCRIBE_TYPE_ONETIME);
    ty_subscribe_event(EVENT_AI_CLIENT_RUN, "ai_desktop", event_netcf_success, SUBSCRIBE_TYPE_ONETIME);
    ty_subscribe_event(EVENT_NETCFG_ERROR, "ai_desktop", event_netcf_fail, SUBSCRIBE_TYPE_ONETIME);
}

static int cfg_network_start(void)
{
    TAL_PR_INFO("[%s] enter", __func__);
    network_start_scr_t *ui = &getContent()->st_startup.network_start_lv;
    ui->cfg_network_scr = lv_obj_create(NULL);
    lv_obj_set_size(ui->cfg_network_scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(ui->cfg_network_scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->cfg_network_scr, lv_color_hex(0x25262A), 0);
    lv_obj_set_style_pad_all(ui->cfg_network_scr, 0, 0);

    lv_obj_t *keyword_text = lv_label_create(ui->cfg_network_scr);
    lv_label_set_text(keyword_text, "配网中");
    lv_obj_set_size(keyword_text, LV_SIZE_CONTENT, 30);
    lv_obj_align(keyword_text, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_font(keyword_text, &AlibabaPuHuiTi3_Regular30, 0);
    lv_obj_set_style_text_color(keyword_text, lv_color_white(), 0);
    lv_obj_set_style_text_align(keyword_text, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_update_layout(ui->cfg_network_scr);
}

static void cfg_network_success_emoji(lv_timer_t * timer)
{
    TAL_PR_INFO("[%s] enter", __func__);

    lv_home_ui_t *home_ui = &getContent()->st_home;

    png_img_unload(&s_netcfg_scr_res.emoj_gif);

    memset(&s_netcfg_scr_res, 0, sizeof(netcfg_scr_res_t));    

    switch_ui_scr_animation(&home_ui->home1_lv.home_scr1, setup_scr_home_scr1, LV_SCR_LOAD_ANIM_NONE, SWITCH_SCREEN_PERMANENT);
    
    lv_timer_del(timer);
}

static int cfg_network_success(void)
{
    TAL_PR_INFO("[%s] enter", __func__);  
    network_start_scr_t *ui = &getContent()->st_startup.network_start_lv;
    OPERATE_RET rt = gif_img_load(tuya_app_gui_get_picture_full_path(GIF_HAPPY_EMOJ), &s_netcfg_scr_res.emoj_gif);

    if (rt == OPRT_OK) 
    {
        lv_obj_t *happy_emoji = lv_gif_create(ui->cfg_network_scr);
        lv_obj_align(happy_emoji, LV_ALIGN_CENTER, 0, 0);
        lv_gif_set_src(happy_emoji, &s_netcfg_scr_res.emoj_gif);
    }
    else
    {
        TAL_PR_ERR("[%s] gif load failed: %d", __func__, rt);
    }

    lv_obj_update_layout(ui->cfg_network_scr);

    lv_timer_t * timer = lv_timer_create(cfg_network_success_emoji, 5000,  NULL);
}

static int cfg_network_failed(void)
{
    TAL_PR_INFO("[%s] enter", __func__);    
}

void desktop_ui_startup(void)
{
    TAL_PR_INFO("[%s] enter", __FUNCTION__); 
    
    initContent();

#if AI_DESKTOP_GUI_DEBUG    
    extern void setup_personal_center_scr(void);
    setup_personal_center_scr();
#else
    setup_scr_startup(); 

    lv_startup_ui_t *ui = &getContent()->st_startup;

    lv_scr_load(ui->startup_scr);  //加载屏幕
#endif    
}