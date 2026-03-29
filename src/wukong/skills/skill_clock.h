#ifndef __WUKONG_CLOCK_H__
#define __WUKONG_CLOCK_H__

#include "tuya_cloud_types.h"
#include "tuya_list.h"
#include "tal_mutex.h"

/*
* TLV format defined: T(2 byte) + L(2 bytes) + V(x bytes)
*/
#define AI_CLOCK_TLV_T_LEN          2
#define AI_CLOCK_TLV_L_LEN          2
#define AI_CLOCK_TLV_TL_LEN         (AI_CLOCK_TLV_T_LEN + AI_CLOCK_TLV_L_LEN)
typedef enum {
    //countdown_timer
    AI_CLOCK_COUNTDOWN_TAG_OPR,
    AI_CLOCK_COUNTDOWN_TAG_HOUR,
    AI_CLOCK_COUNTDOWN_TAG_MINUTE,
    AI_CLOCK_COUNTDOWN_TAG_SECOND,

    //stopwatch_timer
    AI_CLOCK_STOPWATCH_TAG_OPR,

    //pomodoro_timer
    AI_CLOCK_POMODORO_TAG_OPR,
    AI_CLOCK_POMODORO_TAG_WORK_DUR,
    AI_CLOCK_POMODORO_TAG_SHORT_BRK_DUR,
    AI_CLOCK_POMODORO_TAG_LONG_BRK_DUR,

    //schedule
    AI_CLOCK_SCHED_TAG_OPR,
    AI_CLOCK_SCHED_TAG_START_TIME,
    AI_CLOCK_SCHED_TAG_END_TIME,
    AI_CLOCK_SCHED_TAG_LOCATION,
    AI_CLOCK_SCHED_TAG_DESC,
    AI_CLOCK_SCHED_TAG_CATEGORY,

    //schedule_query
    AI_CLOCK_SCHED_QUERY_TAG_METHOD,
    AI_CLOCK_SCHED_QUERY_TAG_START_TIME,
    AI_CLOCK_SCHED_QUERY_TAG_END_TIME,
    AI_CLOCK_SCHED_QUERY_TAG_KEYWORD,
    AI_CLOCK_SCHED_QUERY_TAG_CATEGORY,

    //ending!
    AI_CLOCK_TAG_MAX
} TY_AI_CLOCK_TLV_TAG_TYPE_E;


typedef enum {
    AI_CLOCK_TIMER_OPR_START = 0,
    AI_CLOCK_TIMER_OPR_PAUSE = 1,
    AI_CLOCK_TIMER_OPR_RESUME = 2,
    AI_CLOCK_TIMER_OPR_STOP = 3,
    AI_CLOCK_TIMER_OPR_RESET = 4,
    AI_CLOCK_TIMER_OPR_MAX
} TY_AI_CLOCK_TIMER_OPR_TYPE_E;

typedef enum {
    AI_CLOCK_SCHED_OPR_ADD = 0,
    AI_CLOCK_SCHED_OPR_DELETE = 1,
    AI_CLOCK_SCHED_OPR_UPDATE = 2,
    AI_CLOCK_SCHED_OPR_MAX
} TY_AI_CLOCK_SCHED_OPR_TYPE_E;

typedef enum {
    AI_CLOCK_SCHED_QUERY_BY_TIME = 0,
    AI_CLOCK_SCHED_QUERY_BY_CATEGORY = 1,
    AI_CLOCK_SCHED_QUERY_BY_KEYWORD = 2,
    AI_CLOCK_SCHED_QUERY_BY_MAX
} TY_AI_CLOCK_SCHED_QUERY_METHOD_E;

typedef enum {
    AI_CLOCK_SCHED_CATEGORY_MEETING = 0,
    AI_CLOCK_SCHED_CATEGORY_WORK = 1,
    AI_CLOCK_SCHED_CATEGORY_PERSONAL = 2,
    AI_CLOCK_SCHED_CATEGORY_HEALTH = 3,
    AI_CLOCK_SCHED_CATEGORY_LEARNING = 4,
    AI_CLOCK_SCHED_CATEGORY_SOCIAL = 5,
    AI_CLOCK_SCHED_CATEGORY_OTHER = 6
} TY_AI_CLOCK_SCHED_CATEGORY_E;

typedef struct {
    INT_T work_duration;
    INT_T short_break_duration;
    INT_T long_break_duration;
} TY_AI_CLOCK_POMODORO_TIMER_CFG_T;

typedef struct {
    TIME_T start_time;
    TIME_T end_time;
    CHAR_T *location;
    CHAR_T *description;
    TY_AI_CLOCK_SCHED_CATEGORY_E categories;
} TY_AI_CLOCK_SCHED_CFG_T;

typedef struct {
    TY_AI_CLOCK_SCHED_CATEGORY_E categories;
    TIME_T start_time;
    TIME_T end_time;
    CHAR_T *keyword;
} TY_AI_CLOCK_SCHED_QUERY_CFG_T;

//闹钟/提醒/日程等信息列表数据结构
typedef struct {
    MUTEX_HANDLE mutex;
    LIST_HEAD list_hd;
} TY_AI_CLOCK_LIST_S;

//日程节点元素信息
typedef struct {
    LIST_HEAD node;
    TY_AI_CLOCK_SCHED_CFG_T sched_ele;
} TY_AI_CLOCK_SCHED_ELEMENT_S;

OPERATE_RET wukong_clock_tlv_data_extract(UINT8_T *buff, UINT_T len, TY_AI_CLOCK_TLV_TAG_TYPE_E T, VOID *out_V, BOOL_T out_string);

TIME_T wukong_clock_time_mktime(CHAR_T *iso_8601_time_str);

OPERATE_RET wukong_clock_set_countdown_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, INT_T hours, INT_T minutes, INT_T seconds);

OPERATE_RET wukong_clock_set_stopwatch_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr);

OPERATE_RET wukong_clock_set_pomodoro_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, TY_AI_CLOCK_POMODORO_TIMER_CFG_T *pomodoro);

OPERATE_RET wukong_clock_set_schedule(TY_AI_CLOCK_SCHED_OPR_TYPE_E opr, TY_AI_CLOCK_SCHED_CFG_T *sched);

CHAR_T *wukong_clock_set_schedule_query(TY_AI_CLOCK_SCHED_QUERY_METHOD_E query_method, TY_AI_CLOCK_SCHED_QUERY_CFG_T *sched_query);

OPERATE_RET wukong_clock_schedule_list_register(TY_AI_CLOCK_LIST_S *sched_list);

#endif  // __WUKONG_CLOCK_H__
