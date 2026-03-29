#ifndef __WUKONG_AUDIO_PLAYER_H__
#define __WUKONG_AUDIO_PLAYER_H__

#include "tuya_cloud_types.h"
#include "wukong_ai_agent.h"
#include "wukong_ai_skills.h"
#include "skill_cloudevent.h"
#include "skill_music_story.h"

typedef enum {
    AI_TOY_ALERT_TYPE_POWER_ON,             /* 开机上电播报 */
    AI_TOY_ALERT_TYPE_NOT_ACTIVE,           /* 还未配网 请先配网 */
    AI_TOY_ALERT_TYPE_NETWORK_CFG,          /* 进入配网状态，开始配网 */
    AI_TOY_ALERT_TYPE_NETWORK_CONNECTED,    /* 联网成功 */
    AI_TOY_ALERT_TYPE_NETWORK_FAIL,         /* 联网失败 重试 */
    AI_TOY_ALERT_TYPE_NETWORK_DISCONNECT,   /* 掉网 */
    AI_TOY_ALERT_TYPE_BATTERY_LOW,          /* 电量不足 */
    AI_TOY_ALERT_TYPE_PLEASE_AGAIN,         /* 请再说一遍 */
    AI_TOY_ALERT_TYPE_LONG_KEY_TALK,        /* 长按键对话 */
    AI_TOY_ALERT_TYPE_KEY_TALK,             /* 按键对话 */
    AI_TOY_ALERT_TYPE_WAKEUP_TALK,          /* 唤醒对话 */
    AI_TOY_ALERT_TYPE_RANDOM_TALK,          /* 任意对话 */
    AI_TOY_ALERT_TYPE_WAKEUP,               /* 你好我在*/
    AI_TOY_ALERT_TYPE_MAX,
} TY_AI_TOY_ALERT_TYPE_E;

typedef enum {
    AI_PLAYER_FG = 0,   // frontground player, used to play tts
    AI_PLAYER_BG = 1,   // background player, used to play music
    AI_PLAYER_ALL = 2,  // all player
} TY_AI_TOY_PLAYER_TYPE_E;

OPERATE_RET wukong_audio_play_data(INT_T format, CONST CHAR_T *data, INT_T len);
OPERATE_RET wukong_audio_play_tts_stream(WUKONG_AI_EVENT_TYPE_E type, AI_AUDIO_CODEC_E codec, CHAR_T *data, INT_T len);
OPERATE_RET wukong_audio_play_tts_url(WUKONG_AI_PLAYTTS_T *tts, BOOL_T is_loop);
OPERATE_RET wukong_audio_play_music(WUKONG_AI_MUSIC_T *music);
OPERATE_RET wukong_audio_play_local(CHAR_T *url, CHAR_T *song_name, CHAR_T *artist, INT_T format, INT_T size);
OPERATE_RET wukong_audio_player_stop(TY_AI_TOY_PLAYER_TYPE_E type);
OPERATE_RET wukong_audio_player_set_vol(UINT8_T vol);

OPERATE_RET wukong_audio_player_init();
OPERATE_RET wukong_audio_player_deinit(VOID);
OPERATE_RET wukong_audio_player_set_resume(BOOL_T is_music_continuous);
OPERATE_RET wukong_audio_player_set_replay(BOOL_T is_music_replay);
OPERATE_RET wukong_audio_player_deinit(VOID);
BOOL_T wukong_audio_player_is_playing();

OPERATE_RET wukong_audio_player_alert(TY_AI_TOY_ALERT_TYPE_E type, BOOL_T send_eof);

#endif  // __WUKONG_AUDIO_PLAYER_H__