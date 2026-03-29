#ifndef __DESK_PERSONAL_H__
#define __DESK_PERSONAL_H__

#include "desk_ui_res.h"

typedef struct 
{
    lv_img_dsc_t back_icon;

    lv_img_dsc_t role_icon;
    lv_img_dsc_t arrow_black;
    lv_img_dsc_t arrow_yellow;

    lv_img_dsc_t photo_icon;
    lv_img_dsc_t camera_icon;
    lv_img_dsc_t music_icon;
    lv_img_dsc_t weather_icon;
    lv_img_dsc_t clock_icon;
    lv_img_dsc_t alarm_icon;
    lv_img_dsc_t calendar_icon;
    lv_img_dsc_t record_icon;
    lv_img_dsc_t file_icon;
    lv_img_dsc_t setting_icon;
    
}personal_scr_res_t;

typedef struct
{
    lv_obj_t *personal_scr;
    lv_obj_t *title;
    lv_obj_t *content;

}lv_personal_ui_t;

void setup_personal_center_scr(void);

void personal_center_scr_res_clear(void);

#endif  // __DESK_PERSONAL_H__