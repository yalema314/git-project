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
    //alarm
    AI_CLOCK_ALARM_TAG_OPR,
    AI_CLOCK_ALARM_TAG_OPR_DEL_ALL,     //是否删除所有闹钟(当操作为删除时)
    AI_CLOCK_ALARM_TAG_HOUR,
    AI_CLOCK_ALARM_TAG_MINUTE,
    AI_CLOCK_ALARM_TAG_DETAIL_TIME,     //设置一次性闹钟时的详细时间(包括年月日)
    AI_CLOCK_ALARM_TAG_LABEL,
    AI_CLOCK_ALARM_TAG_FREQ,            //(0=once, 1=daily, 2=custom_weekly)
    AI_CLOCK_ALARM_TAG_RECURRENCE,      /*weekly recurrence day:
                                         *  one bytes:  bit7       bit6       bit5       bit4       bit3       bit2       bit1       bit0
                                         *               x       Saturday    Friday     Thursday  Wednesday   Tuesday    Monday     Sunday
                                         * such as 0x04 is for Tuesday, 0x12 is for Thursday and Monday.
                                        */
    AI_CLOCK_ALARM_TAG_SNOOZE_ENABLE,   //whether snooze is enabled !
    AI_CLOCK_ALARM_TAG_SNOOZE_INTERVAL, //snooze interval time seconds if snooze is enabled!
    AI_CLOCK_ALARM_TAG_SNOOZE_COUNT,    //snooze counts if snooze is enabled!

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
    AI_CLOCK_ALARM_OPR_ADD = 0,
    AI_CLOCK_ALARM_OPR_DELETE = 1,
    AI_CLOCK_ALARM_OPR_UPDATE = 2,
    AI_CLOCK_ALARM_OPR_MAX
} TY_AI_CLOCK_ALARM_OPR_TYPE_E;

typedef enum {
    AI_CLOCK_SCHED_QUERY_BY_TIME = 0,
    AI_CLOCK_SCHED_QUERY_BY_CATEGORY = 1,
    AI_CLOCK_SCHED_QUERY_BY_KEYWORD = 2,
    AI_CLOCK_SCHED_QUERY_BY_MAX
} TY_AI_CLOCK_SCHED_QUERY_METHOD_E;

typedef enum {
    AI_CLOCK_ALARM_RETEAT_ONCE = 0,
    AI_CLOCK_ALARM_RETEAT_DAILY = 1,
    AI_CLOCK_ALARM_RETEAT_WEEKLY = 2,
    AI_CLOCK_ALARM_RETEAT_MAX
} TY_AI_CLOCK_ALARM_REPEAT_E;

typedef enum {
    AI_CLOCK_ALARM_QUERY_BY_TIME = 0,
    AI_CLOCK_ALARM_QUERY_ALL = 1,
    AI_CLOCK_ALARM_QUERY_BY_MAX
} TY_AI_CLOCK_ALARM_QUERY_METHOD_E;

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
    UCHAR_T hours;
    UCHAR_T minutes;
    TIME_T  detail_time;                    //设置一次性闹钟时的详细UTC时间(包括年月日信息)
    CHAR_T *label;
    TY_AI_CLOCK_ALARM_REPEAT_E repeat_sched_freq;   //(0=once, 1=daily, 2=custom_weekly)
    UCHAR_T weekly_recurrence_day;          /*weekly recurrence day:
                                            *  one bytes:  bit7       bit6       bit5       bit4       bit3       bit2       bit1       bit0
                                            *               x       Saturday    Friday     Thursday  Wednesday   Tuesday    Monday     Sunday
                                            * such as 0x04 is for Tuesday, 0x12 is for Thursday and Monday.
                                            */
    BOOL_T  snooze_enabled;                 //whether snooze is enabled or not!
    INT_T   snooze_interval_sec;            //snooze interval time seconds if snooze is enabled!
    UCHAR_T snooze_count;                   //snooze counts if snooze is enabled!
} TY_AI_CLOCK_ALARM_CFG_T;

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

typedef struct {
    TIME_T start_time;
    TIME_T end_time;
} TY_AI_CLOCK_ALARM_QUERY_CFG_T;

//闹钟/提醒/日程等信息列表数据结构
typedef struct {
    MUTEX_HANDLE mutex;
    LIST_HEAD list_hd;
} TY_AI_CLOCK_LIST_S;

//闹钟节点元素信息
typedef struct {
    LIST_HEAD node;
    TY_AI_CLOCK_ALARM_CFG_T alarm_ele;
    TIME_T last_trigger_slot;               //记录上次触发槽位，避免同一闹钟在同一时间窗重复播报
} TY_AI_CLOCK_ALARM_ELEMENT_S;

//日程节点元素信息
typedef struct {
    LIST_HEAD node;
    TY_AI_CLOCK_SCHED_CFG_T sched_ele;
} TY_AI_CLOCK_SCHED_ELEMENT_S;

OPERATE_RET wukong_clock_tlv_data_extract(UINT8_T *buff, UINT_T len, TY_AI_CLOCK_TLV_TAG_TYPE_E T, VOID *out_V, BOOL_T out_string);

TIME_T wukong_clock_time_mktime(CHAR_T *iso_8601_time_str);

OPERATE_RET wukong_clock_alarm_init(VOID);

OPERATE_RET wukong_clock_alarm_deinit(VOID);

OPERATE_RET wukong_clock_set_alarm(TY_AI_CLOCK_ALARM_OPR_TYPE_E opr, BOOL_T delete_all, TY_AI_CLOCK_ALARM_CFG_T *alarm);

OPERATE_RET wukong_clock_countdown_init(VOID);

OPERATE_RET wukong_clock_countdown_deinit(VOID);

OPERATE_RET wukong_clock_set_countdown_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, INT_T hours, INT_T minutes, INT_T seconds);

OPERATE_RET wukong_clock_set_stopwatch_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr);

OPERATE_RET wukong_clock_set_pomodoro_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, TY_AI_CLOCK_POMODORO_TIMER_CFG_T *pomodoro);

OPERATE_RET wukong_clock_set_schedule(TY_AI_CLOCK_SCHED_OPR_TYPE_E opr, TY_AI_CLOCK_SCHED_CFG_T *sched);

CHAR_T *wukong_clock_set_schedule_query(TY_AI_CLOCK_SCHED_QUERY_METHOD_E query_method, TY_AI_CLOCK_SCHED_QUERY_CFG_T *sched_query);

CHAR_T *wukong_clock_set_alarm_query(TY_AI_CLOCK_ALARM_QUERY_METHOD_E query_method, TY_AI_CLOCK_ALARM_QUERY_CFG_T *alarm_query);

OPERATE_RET wukong_clock_schedule_list_register(TY_AI_CLOCK_LIST_S *sched_list);

OPERATE_RET wukong_clock_alarm_list_register(TY_AI_CLOCK_LIST_S *alarm_list);

#endif  // __WUKONG_CLOCK_H__
