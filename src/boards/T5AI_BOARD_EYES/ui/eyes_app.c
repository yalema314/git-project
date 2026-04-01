#include "tuya_ai_display.h"
#include "gui_common.h"
#include "lvgl.h"

LV_IMG_DECLARE(Neutral);
LV_IMG_DECLARE(Fearful);
LV_IMG_DECLARE(Think);
LV_IMG_DECLARE(Confused);
LV_IMG_DECLARE(Disappointed);
LV_IMG_DECLARE(Angry);
LV_IMG_DECLARE(Happy);
LV_IMG_DECLARE(Sad);
LV_IMG_DECLARE(Surprise);
LV_IMG_DECLARE(Touched);

static lv_obj_t *dual_eyes;
static int current_gif_index = -1;

static const gui_emotion_t gif_emotion[] = {
    {&Neutral,         "neutral"      },
    {&Happy,           "loving"       },
    {&Confused,        "confused"     },
    {&Happy,           "delicious"    },
    {&Fearful,         "fearful"      },
    {&Think,           "thinking"     },
    {&Disappointed,    "disappointed" },
    {&Happy,           "happy"        },
    {&Sad,             "sad"          },
    {&Surprise,        "surprise"     },
    {&Angry,           "angry"        },
    {&Touched,         "touch"        },
    /*----------------------------------- */
    // Single-eye aliases for cloud emotion names. Keep all of them on the
    // 160x160 assets because this board only drives one eye screen.
    {&Sad,             "crying"       },
    {&Happy,           "laughing"     },
    {&Happy,           "winking"      },
    {&Confused,        "cool"         },
    {&Confused,        "embarrassed"  },
    {&Surprise,        "shocked"      },
    {&Confused,        "silly"        },
    {&Surprise,        "suprising"    },
    {&Neutral,         "relaxed"      },
    {&Happy,           "sleepy"       },
};

#include "tal_log.h"

void eyes_emotion_flush(const char *emotion)
{
    uint8_t index = 0;

    TAL_PR_DEBUG("eyes_emotion_flush: %s\r\n", emotion);

    index = gui_emotion_find(gif_emotion, CNTSOF(gif_emotion), emotion);

    if (current_gif_index == index) {
        return;
    }

    current_gif_index = index;

    lv_gif_set_src(dual_eyes, gif_emotion[index].source); 
    lv_obj_center(dual_eyes);
}

//! letf red, right blue
void eyes_test_separate_display(void)
{
    lv_obj_t *left_eye;
    lv_obj_t *right_eye;

    lv_obj_t * obj = NULL;
    obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    left_eye = lv_obj_create(obj);
    lv_obj_set_size(left_eye, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_set_pos(left_eye, 0, 0);
    lv_obj_set_style_pad_all(left_eye, 0, 0);
    lv_obj_set_style_border_width(left_eye, 0, 0);
    lv_obj_set_style_bg_color(left_eye, lv_color_hex(0xFF0000), 0);

    right_eye = lv_obj_create(obj);
    lv_obj_set_size(right_eye, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_set_pos(right_eye, 0, 160);
    lv_obj_set_style_pad_all(right_eye, 0, 0);
    lv_obj_set_style_border_width(right_eye, 0, 0);
    lv_obj_set_style_bg_color(right_eye, lv_color_hex(0x0000FF), 0);
}

void tuya_eyes_init(void)
{
    // eyes_test_separate_display();
    lv_obj_t * obj = NULL;
    obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    dual_eyes = lv_gif_create(obj);

    eyes_emotion_flush("neutral");
}


void tuya_eyes_app(TY_DISPLAY_MSG_T *msg)
{
    switch (msg->type)
    {
        
    case TY_DISPLAY_TP_EMOJI:
        eyes_emotion_flush((const char *)msg->data);
        break;    

    case TY_DISPLAY_TP_CHAT_STAT: {
        if (0 == msg->data[0] || 1 == msg->data[0]) {
            eyes_emotion_flush("neutral");
        }
    } break;

    default:
        break;
    }
}
