#include "desk_event_handle.h"
settings_scr_res_t s_settings_res = {0};

static void settings_chinese_btn_clicked(lv_event_t *e)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("Chinese selected");

        setDeskLanguage(DESK_CHINESE);

        lv_indev_wait_release(lv_indev_get_act());

        lv_obj_del(ui->language_cont);

        settings_homepage_create();

        lv_obj_update_layout(ui->settings_scr); 
    }    
}

static void settings_english_btn_clicked(lv_event_t *e)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("English selected");

        setDeskLanguage(DESK_ENGLISH);
        
        lv_indev_wait_release(lv_indev_get_act());

        lv_obj_del(ui->language_cont);

        settings_homepage_create();

        lv_obj_update_layout(ui->settings_scr); 
    }    
}

static void settings_language_back_event(lv_event_t *e)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("[%s] clicked !!!!!!", __func__);

        lv_indev_wait_release(lv_indev_get_act());

        lv_obj_del(ui->language_cont);

        settings_homepage_create();

        lv_obj_update_layout(ui->settings_scr); 
    }       
}

static void settings_language_page_create(void)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    ui->language_cont = lv_obj_create(ui->settings_scr);
    lv_obj_remove_style_all(ui->language_cont);
    lv_obj_set_size(ui->language_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(ui->language_cont, 0, 0);
    lv_obj_set_style_bg_opa(ui->language_cont, LV_OPA_TRANSP, 0);     

        //标题容器
    lv_obj_t *title = lv_obj_create(ui->language_cont);
    lv_obj_remove_style_all(title);
    lv_obj_set_size(title, LV_HOR_RES, 50);
    lv_obj_set_pos(title, 0, 0);
    lv_obj_set_style_bg_opa(title, LV_OPA_TRANSP, 0);

    lv_obj_t *title_name = lv_label_create(title);
    lv_label_set_text(title_name, "语言");
    lv_obj_set_style_text_font(title_name, &AlibabaPuHuiTi3_Regular18, 0);
    lv_obj_set_size(title_name, LV_SIZE_CONTENT, 20);
    lv_obj_align(title_name, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_align(title_name, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_t *back_btn = lv_btn_create(title);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_set_pos(back_btn, 0, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, settings_language_back_event, LV_EVENT_CLICKED, NULL);

    if(png_img_load(tuya_app_gui_get_picture_full_path(ICON_ICON_BACK_24_24), &s_settings_res.back_icon) == 0) 
    {
        lv_obj_t *back_icon = lv_img_create(back_btn);
        lv_img_set_src(back_icon, &s_settings_res.back_icon);
        lv_obj_set_pos(back_icon, 13, 13);
        lv_obj_set_size(back_icon, 24, 24);
    }

    //内容容器
    lv_obj_t *content = lv_obj_create(ui->language_cont);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - 50); 
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_pos(content, 0, 50);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);  
    
    lv_obj_t *chinese_btn = lv_btn_create(content);
    lv_obj_remove_style_all(chinese_btn);
    lv_obj_set_size(chinese_btn, 240, 54);
    lv_obj_set_pos(chinese_btn, 40, 26);
    lv_obj_set_style_bg_opa(chinese_btn, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(chinese_btn, lv_color_hex(0xFFDC7B), 0);
    lv_obj_set_style_radius(chinese_btn, 27, 0);
    lv_obj_add_event_cb(chinese_btn, settings_chinese_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *chinese_text = lv_label_create(chinese_btn);
    lv_label_set_text(chinese_text, "中文");
    lv_obj_set_size(chinese_text, LV_SIZE_CONTENT, 30);
    lv_obj_align(chinese_text, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_font(chinese_text, &AlibabaPuHuiTi3_Regular30, 0);
    lv_obj_set_style_text_color(chinese_text, lv_color_hex(0xFFF37B), 0);
    lv_obj_set_style_text_align(chinese_text, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中
    
    lv_obj_t *english_btn = lv_btn_create(content);
    lv_obj_remove_style_all(english_btn);
    lv_obj_set_size(english_btn, 240, 54);
    lv_obj_set_pos(english_btn, 40, 90);
    lv_obj_set_style_bg_opa(english_btn, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(english_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(english_btn, 27, 0);
    lv_obj_add_event_cb(english_btn, settings_english_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *english_text = lv_label_create(english_btn);
    lv_label_set_text(english_text, "English");
    lv_obj_set_size(english_text, LV_SIZE_CONTENT, 30);
    lv_obj_align(english_text, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_font(english_text, &AlibabaPuHuiTi3_Regular30, 0);
    lv_obj_set_style_text_color(english_text, lv_color_white(), 0);
    lv_obj_set_style_text_align(english_text, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中
    
    setDeskUIIndex(DESKUI_INDEX_SETTINGS_LANGUAGE);
}

static void settings_network_back_event(lv_event_t *e)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("[%s] clicked !!!!!!", __func__);

        lv_indev_wait_release(lv_indev_get_act());

        lv_obj_del(ui->network_cont);

        settings_homepage_create();

        lv_obj_update_layout(ui->settings_scr); 
    }       
}

static void settings_network_page_create(void)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    ui->network_cont = lv_obj_create(ui->settings_scr);
    lv_obj_remove_style_all(ui->network_cont);
    lv_obj_set_size(ui->network_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(ui->network_cont, 0, 0);
    lv_obj_set_style_bg_opa(ui->network_cont, LV_OPA_TRANSP, 0);     

        //标题容器
    lv_obj_t *title = lv_obj_create(ui->network_cont);
    lv_obj_remove_style_all(title);
    lv_obj_set_size(title, LV_HOR_RES, 50);
    lv_obj_set_pos(title, 0, 0);
    lv_obj_set_style_bg_opa(title, LV_OPA_TRANSP, 0);

    lv_obj_t *title_name = lv_label_create(title);
    lv_label_set_text(title_name, "网络连接");
    lv_obj_set_style_text_font(title_name, &AlibabaPuHuiTi3_Regular18, 0);
    lv_obj_set_size(title_name, LV_SIZE_CONTENT, 20);
    lv_obj_align(title_name, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_align(title_name, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_t *back_btn = lv_btn_create(title);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_set_pos(back_btn, 0, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, settings_network_back_event, LV_EVENT_CLICKED, NULL);

    if(png_img_load(tuya_app_gui_get_picture_full_path(ICON_ICON_BACK_24_24), &s_settings_res.back_icon) == 0) 
    {
        lv_obj_t *back_icon = lv_img_create(back_btn);
        lv_img_set_src(back_icon, &s_settings_res.back_icon);
        lv_obj_set_pos(back_icon, 13, 13);
        lv_obj_set_size(back_icon, 24, 24);
    }

    //内容容器
    lv_obj_t *content = lv_obj_create(ui->network_cont);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - 50); 
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_pos(content, 0, 50);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);  
    
    lv_obj_t *qrcode = lv_qrcode_create(content, 90, lv_color_white(), lv_color_black());
    lv_qrcode_update(qrcode, "https://www.tuya.com/cn/", 24);
    lv_obj_set_pos(qrcode, 115, 8);
    lv_obj_set_size(qrcode, 90, 90);

    lv_obj_t *keyword_text_1 = lv_label_create(content);
    lv_label_set_text(keyword_text_1, "扫描二维码");
    lv_obj_set_size(keyword_text_1, LV_SIZE_CONTENT, 20);
    lv_obj_set_pos(keyword_text_1, 110, 98);
    lv_obj_set_style_text_font(keyword_text_1, &AlibabaPuHuiTi3_Regular20, 0);
    lv_obj_set_style_text_color(keyword_text_1, lv_color_white(), 0);
    lv_obj_set_style_text_align(keyword_text_1, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_t *keyword_text_2 = lv_label_create(content);
    lv_label_set_text(keyword_text_2, "下载APP添加设备");
    lv_obj_set_size(keyword_text_2, LV_SIZE_CONTENT, 18);
    lv_obj_set_pos(keyword_text_2, 88, 128);
    lv_obj_set_style_text_font(keyword_text_2, &AlibabaPuHuiTi3_Regular18, 0);
    lv_obj_set_style_text_color(keyword_text_2, lv_color_white(), 0);
    lv_obj_set_style_text_align(keyword_text_2, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    setDeskUIIndex(DESKUI_INDEX_SETTINGS_NETWORK);
    
}

static void settings_back_event(lv_event_t *e)
{
    lv_personal_ui_t *ui = &getContent()->st_personal;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("[%s] clicked !!!!!!", __func__);

        lv_indev_wait_release(lv_indev_get_act());

        settings_scr_res_clear();

        switch_ui_scr_animation(&ui->personal_scr, setup_personal_center_scr, LV_SCR_LOAD_ANIM_FADE_ON, SWITCH_SCREEN_PERMANENT);    
    }       
}

static void settings_network_choice_event(lv_event_t *e)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("[%s] clicked !!!!!!", __func__);

        lv_obj_del(ui->home_cont);

        settings_network_page_create();

        lv_obj_update_layout(ui->settings_scr); 
    }       
}

static void settings_language_choice_event(lv_event_t *e)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("[%s] clicked !!!!!!", __func__);
        
        lv_obj_del(ui->home_cont);

        settings_language_page_create();

        lv_obj_update_layout(ui->settings_scr); 
    }       
}

static void settings_func_list_create(lv_obj_t  **content)
{
    //箭头icon
    int res = png_img_load(tuya_app_gui_get_picture_full_path(ICON_ICON_ARROW_WHITE), &s_settings_res.arrow_icon);

    //网络连接
    lv_obj_t *network_btn = lv_btn_create(*content);
    lv_obj_remove_style_all(network_btn);
    lv_obj_set_size(network_btn, 280, 70);
    lv_obj_set_pos(network_btn, 20, 10);
    lv_obj_set_style_bg_color(network_btn, lv_color_hex(0xB8BDDE), 0);
    lv_obj_set_style_bg_opa(network_btn, LV_OPA_10, 0);
    lv_obj_set_style_radius(network_btn, 16, 0);
    lv_obj_add_event_cb(network_btn, settings_network_choice_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *network_btn_name = lv_label_create(network_btn);
    lv_label_set_text(network_btn_name, "网络连接");
    lv_obj_set_size(network_btn_name, LV_SIZE_CONTENT, 20);
    lv_obj_set_pos(network_btn_name, 60, 16);
    lv_obj_set_style_text_font(network_btn_name, &AlibabaPuHuiTi3_Regular20, 0);
    lv_obj_set_style_text_color(network_btn_name, lv_color_white(), 0);
    lv_obj_set_style_text_align(network_btn_name, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *network_stat = lv_label_create(network_btn);
    lv_label_set_text(network_stat, "已连接");
    lv_obj_set_size(network_stat, LV_SIZE_CONTENT, 16);
    lv_obj_set_pos(network_stat, 60, 38);
    lv_obj_set_style_text_font(network_stat, &AlibabaPuHuiTi3_Regular16, 0);
    lv_obj_set_style_text_color(network_stat, lv_color_white(), 0);
    lv_obj_set_style_text_align(network_stat, LV_TEXT_ALIGN_CENTER, 0);

    if(png_img_load(tuya_app_gui_get_picture_full_path(ICON_WIFI_30_30), &s_settings_res.network_icon) == 0) 
    {
        lv_obj_t *network_icon = lv_img_create(network_btn);
        lv_img_set_src(network_icon, &s_settings_res.network_icon);
        lv_obj_set_pos(network_icon, 20, 20);
        lv_obj_set_size(network_icon, 30, 30);
    }

    if(res == OPRT_OK)
    {
        lv_obj_t *network_arrow = lv_img_create(network_btn);
        lv_img_set_src(network_arrow, &s_settings_res.arrow_icon);
        lv_obj_set_pos(network_arrow, 236, 20);
        lv_obj_set_size(network_arrow, 24, 24);
    }

    //语言选择
    lv_obj_t *language_btn = lv_btn_create(*content);
    lv_obj_remove_style_all(language_btn);
    lv_obj_set_size(language_btn, 280, 70);
    lv_obj_set_pos(language_btn, 20, 90);
    lv_obj_set_style_bg_color(language_btn, lv_color_hex(0xB8BDDE), 0);
    lv_obj_set_style_bg_opa(language_btn, LV_OPA_10, 0);
    lv_obj_set_style_radius(language_btn, 16, 0);
    lv_obj_add_event_cb(language_btn, settings_language_choice_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *language_btn_name = lv_label_create(language_btn);
    lv_label_set_text(language_btn_name, "语言");
    lv_obj_set_size(language_btn_name, LV_SIZE_CONTENT, 20);
    lv_obj_set_pos(language_btn_name, 60, 16);
    lv_obj_set_style_text_font(language_btn_name, &AlibabaPuHuiTi3_Regular20, 0);
    lv_obj_set_style_text_color(language_btn_name, lv_color_white(), 0);
    lv_obj_set_style_text_align(language_btn_name, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *language_stat = lv_label_create(language_btn);
    if(DESK_CHINESE == getDeskLanguage())
    {
        lv_label_set_text(language_stat, "中文");
    }
    else if(DESK_ENGLISH == getDeskLanguage())
    {
        lv_label_set_text(language_stat, "English");
    }
    lv_obj_set_size(language_stat, LV_SIZE_CONTENT, 16);
    lv_obj_set_pos(language_stat, 60, 38);
    lv_obj_set_style_text_font(language_stat, &AlibabaPuHuiTi3_Regular16, 0);
    lv_obj_set_style_text_color(language_stat, lv_color_white(), 0);
    lv_obj_set_style_text_align(language_stat, LV_TEXT_ALIGN_CENTER, 0);

    if(png_img_load(tuya_app_gui_get_picture_full_path(ICON_ICON_NETWORK_30_30), &s_settings_res.language_icon) == 0) 
    {
        lv_obj_t *language_icon = lv_img_create(language_btn);
        lv_img_set_src(language_icon, &s_settings_res.language_icon);
        lv_obj_set_pos(language_icon, 20, 20);
        lv_obj_set_size(language_icon, 30, 30);
    }

    if(res == OPRT_OK)
    {
        lv_obj_t *language_arrow = lv_img_create(language_btn);
        lv_img_set_src(language_arrow, &s_settings_res.arrow_icon);
        lv_obj_set_pos(language_arrow, 236, 20);
        lv_obj_set_size(language_arrow, 24, 24);
    }
}

static void settings_homepage_create(void)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;
    ui->home_cont = lv_obj_create(ui->settings_scr);
    lv_obj_remove_style_all(ui->home_cont);
    lv_obj_set_size(ui->home_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(ui->home_cont, 0, 0);
    lv_obj_set_style_bg_opa(ui->home_cont, LV_OPA_TRANSP, 0);     

        //标题容器
    lv_obj_t *title = lv_obj_create(ui->home_cont);
    lv_obj_remove_style_all(title);
    lv_obj_set_size(title, LV_HOR_RES, 50);
    lv_obj_set_pos(title, 0, 0);
    lv_obj_set_style_bg_opa(title, LV_OPA_TRANSP, 0);

    lv_obj_t *title_name = lv_label_create(title);
    lv_label_set_text(title_name, "设置");
    lv_obj_set_style_text_font(title_name, &AlibabaPuHuiTi3_Regular18, 0);
    lv_obj_set_size(title_name, LV_SIZE_CONTENT, 20);
    lv_obj_align(title_name, LV_ALIGN_CENTER, 0, 0);    //相对于父对象居中
    lv_obj_set_style_text_align(title_name, LV_TEXT_ALIGN_CENTER, 0);   //label内部文本居中

    lv_obj_t *back_btn = lv_btn_create(title);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_set_pos(back_btn, 0, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, settings_back_event, LV_EVENT_CLICKED, NULL);

    if(png_img_load(tuya_app_gui_get_picture_full_path(ICON_ICON_BACK_24_24), &s_settings_res.back_icon) == 0) 
    {
        lv_obj_t *back_icon = lv_img_create(back_btn);
        lv_img_set_src(back_icon, &s_settings_res.back_icon);
        lv_obj_set_pos(back_icon, 13, 13);
        lv_obj_set_size(back_icon, 24, 24);
    }

    //内容容器
    lv_obj_t *content = lv_obj_create(ui->home_cont);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - 50); 
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_pos(content, 0, 50);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);    

    settings_func_list_create(&content);
}

void setup_settings_scr(void)
{
    lv_settings_ui_t *ui = &getContent()->st_func_settings;

    ui->settings_scr = lv_obj_create(NULL);
    lv_obj_set_size(ui->settings_scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(ui->settings_scr, lv_color_hex(0x25262A), 0);
    lv_obj_set_style_pad_all(ui->settings_scr, 0, 0);
    lv_obj_set_style_text_font(ui->settings_scr, &AlibabaPuHuiTi3_Regular16, 0);
    lv_obj_set_style_text_color(ui->settings_scr, lv_color_white(), 0);
    lv_obj_set_scrollbar_mode(ui->settings_scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(ui->settings_scr, LV_DIR_NONE);

    settings_homepage_create();

    lv_obj_update_layout(ui->settings_scr);

    setDeskUIIndex(DESKUI_INDEX_SETTINGS);
}

void settings_scr_res_clear(void)
{
    TAL_PR_INFO("[%s] enter.", __func__);

    //清除图片资源
    png_img_unload(&s_settings_res.back_icon);
    png_img_unload(&s_settings_res.arrow_icon);
    png_img_unload(&s_settings_res.network_icon);
    png_img_unload(&s_settings_res.language_icon);

    memset(&s_settings_res, 0, sizeof(chat_scr_res_t));
}