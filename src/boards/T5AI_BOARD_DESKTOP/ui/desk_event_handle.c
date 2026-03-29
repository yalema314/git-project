#include "desk_event_handle.h"
#include "tuya_iot_com_api.h"
#include "base_event.h"
#include "base_event_info.h"
#include "tuya_error_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static desk_ui_lv_t *s_lv_ui_handle = {0};
static bool is_chat_screen = false;
lv_font_t *AlibabaPuHuiTi3_55_18 = NULL;

static GIF_EMOJ_T s_gif_emoj_table[] = 
{
    {GIF_DEFAULT,      GIF_DEFAULT_EMOJ,        GIF_DEFAULT_ORI_EMOJ,       "neutral"},
    {GIF_HAPPY,        GIF_HAPPY_EMOJ,          GIF_HAPPY_ORI_EMOJ,         "happy"},
    {GIF_CONFUSED,     GIF_CONFUSED_EMOJ,       GIF_CONFUSED_ORI_EMOJ,      "confused"},
    {GIF_SHY,          GIF_SHY_EMOJ,            GIF_SHY_ORI_EMOJ,           "sleepy"},
    {GIF_CRY,          GIF_CRY_EMOJ,            GIF_CRY_ORI_EMOJ,           "crying"},
    {GIF_ANGRY,        GIF_ANGRY_EMOJ,          GIF_ANGRY_ORI_EMOJ,         "angry"},
    {GIF_SCARED,       GIF_SCARED_EMOJ,         GIF_SCARED_ORI_EMOJ,        "shocked"},
    {GIF_SURPRISED,    GIF_SURPRISED_EMOJ,      GIF_SURPRISED_ORI_EMOJ,     "surprise"},
    {GIF_DISAPPOINTED, GIF_DISAPPOINTED_EMOJ,   GIF_DISAPPOINTED_ORI_EMOJ,  "sad"}, 
    {GIF_ANNOYED,      GIF_ANNOYED_EMOJ,        GIF_ANNOYED_ORI_EMOJ,       "silly"},
    {GIF_THINKING,     GIF_THINKING_EMOJ,       GIF_THINKING_ORI_EMOJ,      "thinking"},
    {GIF_LAUGH,        GIF_LAUGH_EMOJ,          GIF_LAUGH_ORI_EMOJ,         "laughing"},
    {GIF_FUNNY,        GIF_FUNNY_EMOJ,          GIF_FUNNY_ORI_EMOJ,         "funny"},
    {GIF_LOVE,         GIF_LOVE_EMOJ,           GIF_LOVE_ORI_EMOJ,          "loving"},
    {GIF_EMBARRASSED,  GIF_EMBARRASSED_EMOJ,    GIF_EMBARRASSED_ORI_EMOJ,   "embarrassed"},
    {GIF_BLINK,        GIF_BLINK_EMOJ,          GIF_BLINK_ORI_EMOJ,         "winking"},       
    {GIF_COOL,         GIF_COOL_EMOJ,           GIF_COOL_ORI_EMOJ,          "cool"},
    {GIF_RELAXED,      GIF_RELAXED_EMOJ,        GIF_RELAXED_ORI_EMOJ,       "relaxed"},
    {GIF_DELICIOUS,    GIF_DELICIOUS_EMOJ,      GIF_DELICIOUS_ORI_EMOJ,     "delicious"},
    {GIF_UNHAPPY,      GIF_UNHAPPY_EMOJ,        GIF_UNHAPPY_ORI_EMOJ,       "sad"},  
};

desk_ui_lv_t *getContent()
{
    return s_lv_ui_handle;
}

void initContent()
{    
    AlibabaPuHuiTi3_55_18 = lv_font_load(FONT_PUHUITI3_55_TTF);
    if(AlibabaPuHuiTi3_55_18)
    {
        TAL_PR_INFO("[%s] lv_font_load success", __func__);
    }

    s_lv_ui_handle = tal_psram_malloc(sizeof(desk_ui_lv_t));
    if(s_lv_ui_handle == NULL)
    {
        TAL_PR_ERR("[%s] malloc fail", __func__);
        return;
    }

    memset(s_lv_ui_handle, 0, sizeof(desk_ui_lv_t));  

    s_lv_ui_handle->st_ai_message.asr_txt = (char *)tal_psram_malloc(AI_ASR_MESSAGE_LEN);
    if(s_lv_ui_handle->st_ai_message.asr_txt == NULL)
    {
        TAL_PR_ERR("[%s] asr txt malloc fail", __func__);
    }

    s_lv_ui_handle->st_ai_message.tts_txt = (char *)tal_psram_malloc(AI_TTS_MESSAGE_LEN);
    if(s_lv_ui_handle->st_ai_message.tts_txt == NULL)
    {
        TAL_PR_ERR("[%s] tts txt malloc fail", __func__);
    }

    s_lv_ui_handle->active_stat = get_gw_active();

    initDeskLanguage();
}

char *getGifEmojNameByIndex(GIF_EMOJ_E index, bool ori)
{
    for(size_t i = 0; i < sizeof(s_gif_emoj_table)/sizeof(GIF_EMOJ_T); i++)
    {
        if(s_gif_emoj_table[i].index == index)
        {
            if(ori)
            {
                return s_gif_emoj_table[i].ori_gif_name;
            }
            else
            {
                return s_gif_emoj_table[i].gif_name;
            }
        }
    }

    return NULL;
}

int initDeskLanguage()
{
    OPERATE_RET rt = OPRT_OK;
    BYTE_T *value = NULL;
    UINT_T len = 0;

    s_lv_ui_handle->language_stat = DESK_ENGLISH;

    // read volume from kv
    TUYA_CALL_ERR_RETURN(wd_common_read(AI_DESKTOP_LANGUAGE, &value, &len));
    TAL_PR_DEBUG("read desk language config: %s", value);
    ty_cJSON *root = ty_cJSON_Parse((CONST CHAR_T *)value);
    wd_common_free_data(value);
    TUYA_CHECK_NULL_RETURN(root, OPRT_FILE_READ_FAILED);

    // read volum
    ty_cJSON *pdata = ty_cJSON_GetObjectItem(root, "desk_language");
    if (pdata) {
        if (pdata->valueint == DESK_CHINESE || pdata->valueint == DESK_ENGLISH) {
            s_lv_ui_handle->language_stat = pdata->valueint;
        }
    }

    ty_cJSON_Delete(root);
}

int setDeskLanguage(int value)
{
    int rt = OPRT_OK;
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "{\"desk_language\": %d}", value);
    TUYA_CALL_ERR_RETURN(wd_common_write(AI_DESKTOP_LANGUAGE, (CONST BYTE_T *)buf, strlen(buf)));
    TAL_PR_DEBUG("save desk language config: %s", buf);
    s_lv_ui_handle->language_stat = value;
    return rt;
}

int getDeskLanguage()
{
    return s_lv_ui_handle->language_stat;
}

void setDeskUIIndex(DESKUI_INDEX index)
{
    s_lv_ui_handle->deskui_index = index;
}

INT_T getDeskUIIndex()
{
    return s_lv_ui_handle->deskui_index;
}

void switch_ui_scr_animation(lv_obj_t ** new_scr, ui_setup_scr_cb setup_scr, lv_scr_load_anim_t anim_type, SWITCH_SCREEN_TYPE_E del_type)
{
    lv_obj_t * act_scr = lv_scr_act();
    
    switch (del_type)
    {
        case SWITCH_SCREEN_PERMANENT:
        {
            lv_obj_clean(act_scr);

            if(setup_scr)
            {
                setup_scr();
            }

            lv_scr_load_anim(*new_scr, anim_type, SWITCH_UI_DURATION, SWITCH_UI_DELAY, true);
        }
        break;

        case SWITCH_SCREEN_TEMPORARY:
        {
            setup_scr();
            lv_scr_load_anim(*new_scr, anim_type, SWITCH_UI_DURATION, SWITCH_UI_DELAY, false);
        }
        break;

        case SWITCH_SCREEN_DYNAMIC:
        {
            lv_obj_clean(act_scr);

            if(setup_scr)
            {
                setup_scr();
            }

            lv_scr_load_anim(*new_scr, anim_type, SWITCH_UI_DURATION, SWITCH_UI_DELAY, false);
        }
        break;
        
        default:
        break;
    }
}

void handle_home1_event(lv_event_t *e)
{
    static unsigned int first_time = 0;
    static unsigned int second_time = 0;
    static int gif_index = 0;
    lv_home_ui_t *ui = &getContent()->st_home;
    lv_chat_ui_t *chat_ui = &getContent()->st_chat;
    lv_event_code_t code = lv_event_get_code(e);
    char gif_path[32] = {0};

    if(code == LV_EVENT_GESTURE)
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        if(dir == LV_DIR_LEFT)
        {
            TAL_PR_DEBUG("[%s] left", __func__);

            lv_indev_wait_release(lv_indev_get_act());

            home_scr1_res_clear();

            gif_index = 0;

            switch_ui_scr_animation(&ui->home2_lv.home_scr2, setup_scr_home_scr2, LV_SCR_LOAD_ANIM_MOVE_LEFT, SWITCH_SCREEN_DYNAMIC);
        }
        else if(dir == LV_DIR_RIGHT)
        {
            TAL_PR_DEBUG("[%s] right", __func__);
            lv_indev_wait_release(lv_indev_get_act());

            home_scr1_res_clear();

            gif_index = 0;

            switch_ui_scr_animation(&chat_ui->main_cont, setup_scr_chat_scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, SWITCH_SCREEN_DYNAMIC);

            is_chat_screen = true;
        }
        else if(dir == LV_DIR_BOTTOM)
        {
            TAL_PR_DEBUG("[%s] bottom", __func__);

            lv_indev_wait_release(lv_indev_get_act());

            home_scr1_res_clear();

            gif_index = 0;

            switch_ui_scr_animation(&ui->home3_lv.home_scr3, setup_scr_home_scr3, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, SWITCH_SCREEN_DYNAMIC);

            ui->home3_lv.last_screen = HOME_SCREEN_INDEX1;
        }
    }
    else if(code == LV_EVENT_CLICKED)
    {
        if(first_time == 0) //first_time=0, 代表第一次单击
        {
            first_time = tal_system_get_millisecond();
        }
        else
        {
            second_time = tal_system_get_millisecond();
            if(second_time - first_time <= CLICKED_EVENT_TIME)
            {
                gif_index = gif_index + 1 % GIF_MAX;
                strncpy(gif_path, getGifEmojNameByIndex(gif_index, false), sizeof(gif_path));
                home1_gif_switch(gif_path, false);

            }
            first_time = 0;
            second_time = 0;        
        }
    }    
}

void handle_home2_event(lv_event_t *e)
{
    lv_home_ui_t *ui = &getContent()->st_home;
    lv_personal_ui_t *personal_center = &getContent()->st_personal;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_GESTURE)
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        if(dir == LV_DIR_LEFT)
        {
            lv_indev_wait_release(lv_indev_get_act());

            home_scr2_res_clear();

            switch_ui_scr_animation(&personal_center->personal_scr, setup_personal_center_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, SWITCH_SCREEN_DYNAMIC);
        }
        else if(dir == LV_DIR_RIGHT)
        {
            lv_indev_wait_release(lv_indev_get_act());

            home_scr2_res_clear();

            switch_ui_scr_animation(&ui->home1_lv.home_scr1, setup_scr_home_scr1, LV_SCR_LOAD_ANIM_MOVE_RIGHT, SWITCH_SCREEN_DYNAMIC);
        }
        else if(dir == LV_DIR_BOTTOM)
        {
            lv_indev_wait_release(lv_indev_get_act());

            home_scr2_res_clear();

            switch_ui_scr_animation(&ui->home3_lv.home_scr3, setup_scr_home_scr3, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, SWITCH_SCREEN_DYNAMIC);

            ui->home3_lv.last_screen = HOME_SCREEN_INDEX2;
        }
    }    
}

void handle_home3_event(lv_event_t *e)
{
    lv_home_ui_t *ui = &getContent()->st_home;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if((code == LV_EVENT_GESTURE) && (target == ui->home3_lv.home_scr3))
    {
        TAL_PR_INFO("[%s] gesture", __func__);
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        if(dir == LV_DIR_TOP)
        {
            lv_indev_wait_release(lv_indev_get_act());

            home_scr3_res_clear();

            if(ui->home3_lv.last_screen == HOME_SCREEN_INDEX2)
            {
                switch_ui_scr_animation(&ui->home2_lv.home_scr2, setup_scr_home_scr2, LV_SCR_LOAD_ANIM_MOVE_TOP, SWITCH_SCREEN_DYNAMIC);            
            }    
            else
            {
                switch_ui_scr_animation(&ui->home1_lv.home_scr1, setup_scr_home_scr1, LV_SCR_LOAD_ANIM_MOVE_TOP, SWITCH_SCREEN_DYNAMIC);            
            }      
        }
    }
}

void handle_home3_slider_event(lv_event_t *e)
{
    lv_home_ui_t *ui = &getContent()->st_home;
    lv_event_code_t code = lv_event_get_code(e);

    typedef enum
    {
        IDLE_SLIDER = 0,
        VOLUME_SLIDER,
        BRIGHTNESS_SLIDER,
        CLOCK_VOLUME_SLIDER
    }slider_type_e;

    typedef struct
    {
        slider_type_e slider_type;
        unsigned int volume_v;
        unsigned int brightness_v;
        unsigned int clock_volume_v;
    }device_control_t;

    static device_control_t device_control = {0}; 

    if (code == LV_EVENT_PRESSED) 
    {
        lv_obj_remove_event_cb(ui->home3_lv.home_scr3, handle_home3_event);
    }
    else if (code == LV_EVENT_RELEASED) 
    {
        lv_obj_add_event_cb(ui->home3_lv.home_scr3, handle_home3_event, LV_EVENT_GESTURE, NULL);
        
        if (device_control.slider_type == VOLUME_SLIDER) {
            TAL_PR_DEBUG("[%s] set volume: %d", __func__, device_control.volume_v);
            device_control.slider_type = IDLE_SLIDER;
            tuya_ai_toy_volume_set(device_control.volume_v);

        } else if(device_control.slider_type == BRIGHTNESS_SLIDER) {
            TAL_PR_DEBUG("[%s] set brightness: %d", __func__, device_control.brightness_v);
            device_control.slider_type = IDLE_SLIDER;

        } else if(device_control.slider_type == CLOCK_VOLUME_SLIDER) {
            TAL_PR_DEBUG("[%s] set clock volume: %d", __func__, device_control.clock_volume_v);
            device_control.slider_type = IDLE_SLIDER;

        }
    }

    if (code == LV_EVENT_VALUE_CHANGED) 
    {
        lv_obj_t *target = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(target);

        if(target == ui->home3_lv.home_scr3_volume_sli)
        {
            device_control.slider_type = VOLUME_SLIDER;
            device_control.volume_v = value;
        }
        else if(target == ui->home3_lv.home_scr3_brightness_sli)
        {
            device_control.slider_type = BRIGHTNESS_SLIDER;
            device_control.brightness_v = value;
        }
        else if(target == ui->home3_lv.home_scr3_clock_vol_sli)
        {
            device_control.slider_type = CLOCK_VOLUME_SLIDER;
            device_control.clock_volume_v = value;
        }
    }
}

void handle_home3_clicked_event(lv_event_t *e)
{
    lv_home_ui_t *ui = &getContent()->st_home;
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("[%s] clicked !!!!!!", __func__);

        lv_indev_wait_release(lv_indev_get_act());

        home_scr3_res_clear();

        if(ui->home3_lv.last_screen == HOME_SCREEN_INDEX2)
        {
            switch_ui_scr_animation(&ui->home2_lv.home_scr2, setup_scr_home_scr2, LV_SCR_LOAD_ANIM_MOVE_TOP, SWITCH_SCREEN_DYNAMIC);            
        }    
        else
        {
            switch_ui_scr_animation(&ui->home1_lv.home_scr1, setup_scr_home_scr1, LV_SCR_LOAD_ANIM_MOVE_TOP, SWITCH_SCREEN_DYNAMIC);            
        }   
    }       
}


void handle_chat_event(lv_event_t *e)
{
    lv_home_ui_t *home_ui = &getContent()->st_home;
    lv_chat_ui_t *chat_ui = &getContent()->st_chat;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if(code == LV_EVENT_GESTURE)
    {
        TAL_PR_INFO("[%s] gesture", __func__);
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        if(dir == LV_DIR_LEFT)
        {
            is_chat_screen = false;

            lv_indev_wait_release(lv_indev_get_act());

            chat_scr_res_clear();

            switch_ui_scr_animation(&home_ui->home1_lv.home_scr1, setup_scr_home_scr1, LV_SCR_LOAD_ANIM_MOVE_LEFT, SWITCH_SCREEN_PERMANENT);                          
        }
    }
}

void handle_personal_center_back_event(lv_event_t *e)
{
    lv_home_ui_t *home_ui = &getContent()->st_home;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) 
    {
        TAL_PR_DEBUG("[handle_personal_center_back_event] clicked !!!!!!");

        lv_indev_wait_release(lv_indev_get_act());

        personal_center_scr_res_clear();

        switch_ui_scr_animation(&home_ui->home2_lv.home_scr2, setup_scr_home_scr2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, SWITCH_SCREEN_PERMANENT);    
    }       
}

void receive_ai_message_data(TY_DISPLAY_TYPE_E type, char *data, int len)
{

    static lv_obj_t *label = NULL;
    static lv_obj_t *parent = NULL;
    static bool is_show_message = false;
    lv_chat_ui_t *chat_ui = &getContent()->st_chat;
    int buff_len = 0;

    if(!is_chat_screen)
    {        
        TAL_PR_ERR("[receive_ai_message_data] current screen not ai chat.");

        if(parent != NULL)
        {
            parent = NULL;
        }

        if(label != NULL)
        {
            label = NULL;
        }

        is_show_message = false;
        return;
    }

    if(s_lv_ui_handle->st_ai_message.asr_txt == NULL || s_lv_ui_handle->st_ai_message.tts_txt == NULL)
    {
        TAL_PR_ERR("[receive_ai_message_data] ai chat buffer is null.");
        is_show_message = false;
        return;
    }

    switch(type)
    {
        case TY_DISPLAY_TP_HUMAN_CHAT:
        {
            TAL_PR_ERR("[receive_ai_message_data] TY_DISPLAY_TP_HUMAN_CHAT.");
            memset(s_lv_ui_handle->st_ai_message.asr_txt, 0, AI_ASR_MESSAGE_LEN); 
            if(len < AI_ASR_MESSAGE_LEN && len > 0)
            {
                // memcpy(s_lv_ui_handle->st_ai_message.asr_txt, data, len);
                strncpy(s_lv_ui_handle->st_ai_message.asr_txt, data, len);
                s_lv_ui_handle->st_ai_message.asr_len = len;
                set_chat_message(s_lv_ui_handle->st_ai_message.asr_txt, false);
            }

        }
        break;

        case TY_DISPLAY_TP_AI_CHAT_START:
        {
            TAL_PR_ERR("[receive_ai_message_data] TY_DISPLAY_TP_AI_CHAT_START.");
            TAL_PR_ERR("[receive_ai_message_data] tts: %s, len:%d.", data, len);
            memset(s_lv_ui_handle->st_ai_message.tts_txt, 0, AI_TTS_MESSAGE_LEN);

            // memcpy(s_lv_ui_handle->st_ai_message.tts_txt, data, len);
            // strncpy(s_lv_ui_handle->st_ai_message.tts_txt, data + 3, len);
            strncpy(s_lv_ui_handle->st_ai_message.tts_txt, data, len);
            s_lv_ui_handle->st_ai_message.tts_len +=  len;

            parent = create_chat_message(&label, true);

            is_show_message = true;

            lv_label_set_text(label, s_lv_ui_handle->st_ai_message.tts_txt);
            lv_obj_scroll_to_view(parent, LV_ANIM_ON);
            lv_obj_update_layout(chat_ui->msg_container);

        }
        break;

        case TY_DISPLAY_TP_AI_CHAT_DATA:
        { 
            TAL_PR_ERR("[receive_ai_message_data] TY_DISPLAY_TP_AI_CHAT_DATA.");
            TAL_PR_ERR("[receive_ai_message_data] tts: %s, len:%d.", data, len);
            buff_len = s_lv_ui_handle->st_ai_message.tts_len + len;
            if(buff_len >= AI_TTS_MESSAGE_LEN)
            {
                TAL_PR_ERR("[receive_ai_message_data] tts chat buffer is overflow.");
                break;
            }

            strncat(s_lv_ui_handle->st_ai_message.tts_txt, data, len);
            s_lv_ui_handle->st_ai_message.tts_len += len;

            if(!is_show_message)
            {
                TAL_PR_ERR("[receive_ai_message_data] parent and obj not creat.");
                break;
            }

            TAL_PR_ERR("[receive_ai_message_data] show ai chat tts data.");
            lv_label_set_text(label, s_lv_ui_handle->st_ai_message.tts_txt);
            lv_obj_scroll_to_view(parent, LV_ANIM_ON);
            lv_obj_update_layout(chat_ui->msg_container);

        }
        break;
        
        default:
        break;
    }
}

void receive_ai_chat_mode_data(char *data, int len)
{
    if(NULL == data || len <= 0)
    {
        TAL_PR_ERR("[%s] input error", __func__);
        return;
    }

    int trigger_mode = data[0];
    TAL_PR_INFO("[receive_ai_message_data] ai chat %d ", trigger_mode);
    setup_scr_chat_mode(trigger_mode);                    
}

void receive_emotional_feedback(char *data, int len)
{
    if(NULL == data || len <= 0)
    {
        TAL_PR_ERR("[%s] input error", __func__);
        return;
    }

    DESKUI_INDEX deskui_index = getDeskUIIndex();
    if(deskui_index != DESKUI_INDEX_HOME1)
    {
        TAL_PR_INFO("[%s] current desk ui index: %d", __func__, deskui_index);
        return;
    }

    for(size_t i = 0; i < sizeof(s_gif_emoj_table)/sizeof(GIF_EMOJ_T); i++)
    {
        if(strcmp(s_gif_emoj_table[i].emo_name, data) == 0)
        {
            TAL_PR_INFO("[%s] current ui emo name: %s", __func__, data);
            home1_gif_switch(s_gif_emoj_table[i].gif_name, true);
        }
    }

}

