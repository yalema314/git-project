#ifndef __DESK_CHAT_H__
#define __DESK_CHAT_H__ 
#include "desk_ui_res.h"

typedef struct 
{
    lv_img_dsc_t ai_icon;
}chat_scr_res_t;

typedef struct
{
	lv_obj_t *main_cont;
	lv_obj_t *ai_icon;
    lv_obj_t *msg_container;
}lv_chat_ui_t;

void setup_scr_chat_scr(void);

void chat_scr_res_clear(void);

void set_chat_message(uint8_t *data, bool is_ai);

lv_obj_t* create_chat_message(lv_obj_t **lable, bool is_ai);

void setup_scr_chat_mode(int mode);


#endif  // __DESK_CHAT_H__