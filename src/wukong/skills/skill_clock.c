#include "skill_clock.h"
#include "wukong_ai_agent.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_sw_timer.h"
#include "tal_time_service.h"
#include "wukong_audio_player.h"
#include "mix_method.h"
#include <stdio.h>

STATIC TY_AI_CLOCK_LIST_S *__SchedList = NULL;
STATIC TY_AI_CLOCK_LIST_S *__AlarmList = NULL;
STATIC TY_AI_CLOCK_LIST_S __AlarmListStorage = {0};
STATIC BOOL_T __AlarmListOwned = FALSE;
STATIC TIMER_ID __AlarmTimer = NULL;
STATIC TIMER_ID __AlarmSpeakTimer = NULL;
STATIC TIMER_ID __CountdownTimer = NULL;
STATIC MUTEX_HANDLE __CountdownMutex = NULL;
STATIC TIME_T __CountdownDeadline = 0;
STATIC TIME_T __CountdownRemainSec = 0;
STATIC BOOL_T __CountdownRunning = FALSE;
/* 缓存下一次 AI 播报的提示词，给英文提醒和标签内容预留更充足的空间。 */
STATIC CHAR_T __AlarmSpeakPrompt[384] = {0};

#define AI_CLOCK_ALARM_CHECK_PERIOD_MS 1000
#define AI_CLOCK_ALARM_SPEAK_DELAY_MS  2000
#define AI_CLOCK_ALARM_PROMPT_MAX_LEN  ((INT_T)SIZEOF(__AlarmSpeakPrompt))
#define ALARM_MALLOC_SIZE              256
#define ALARM_MAX_NUM                  50
#define ALARM_CONTENT_HEAD             "Query Results: Found 255 alarms\n"

STATIC VOID __wukong_clock_alarm_check_timer_cb(TIMER_ID timer_id, VOID_T *arg);
STATIC VOID __wukong_clock_countdown_timer_cb(TIMER_ID timer_id, VOID_T *arg);
STATIC VOID __wukong_clock_alarm_speak_timer_cb(TIMER_ID timer_id, VOID_T *arg);

STATIC VOID wukong_clock_tlv_data_pack(UINT8_T *buff, UINT_T T, UINT_T L, UINT8_T *V, UINT_T *offset)
{
    UINT8_T tmp_t1 = 0, tmp_l1 = 0;
    UINT16_T tmp_t2 = 0, tmp_l2 = 0;
    
    if (AI_CLOCK_TLV_T_LEN == 1) {
        tmp_t1 = (UINT8_T)T;
        memcpy(buff + *offset, &tmp_t1, AI_CLOCK_TLV_T_LEN);
    }
    else if (AI_CLOCK_TLV_T_LEN == 2) {
        tmp_t2 = (UINT16_T)T;
        memcpy(buff + *offset, &tmp_t2, AI_CLOCK_TLV_T_LEN);
    }
    else {
        TAL_PR_ERR("%s: unsupport TLV T len '%d' !", __func__, AI_CLOCK_TLV_T_LEN);
        return;
    }
    *offset += AI_CLOCK_TLV_T_LEN;

    if (AI_CLOCK_TLV_L_LEN == 1) {
        tmp_l1 = (UINT8_T)L;
        memcpy(buff + *offset, &tmp_l1, AI_CLOCK_TLV_L_LEN);
    }
    else if (AI_CLOCK_TLV_L_LEN == 2) {
        tmp_l2 = (UINT16_T)L;
        memcpy(buff + *offset, &tmp_l2, AI_CLOCK_TLV_L_LEN);
    }
    else {
        TAL_PR_ERR("%s: unsupport TLV L len '%d' !", __func__, AI_CLOCK_TLV_L_LEN);
        return;
    }
    *offset += AI_CLOCK_TLV_L_LEN;

    if (L > 0) {
        memcpy(buff + *offset, V, L);
        *offset += L;
    }
}

STATIC VOID wukong_clock_tlv_data_packed_show(UINT8_T *buff, INT_T len)
{
    TAL_PR_HEXDUMP_DEBUG("clock-tlv", buff, len);
}

OPERATE_RET wukong_clock_tlv_data_extract(UINT8_T *buff, UINT_T len, TY_AI_CLOCK_TLV_TAG_TYPE_E T, VOID *out_V, BOOL_T out_string)
{
    OPERATE_RET rt = OPRT_COM_ERROR;
    UINT8_T tmp_t1 = 0, tmp_l1 = 0;
    UINT16_T tmp_t2 = 0, tmp_l2 = 0;
    UINT_T offset = 0;

    while (offset < len) {
        //parse T'
        if (AI_CLOCK_TLV_T_LEN == 1) {
            memcpy(&tmp_t1, buff + offset, AI_CLOCK_TLV_T_LEN);
        }
        else if (AI_CLOCK_TLV_T_LEN == 2) {
            memcpy(&tmp_t2, buff + offset, AI_CLOCK_TLV_T_LEN);
        }
        else {
            TAL_PR_ERR("%s: unsupport TLV T len '%d' !", __func__, AI_CLOCK_TLV_T_LEN);
            return rt;
        }
        offset += AI_CLOCK_TLV_T_LEN;

        //parse L'
        if (AI_CLOCK_TLV_L_LEN == 1) {
            memcpy(&tmp_l1, buff + offset, AI_CLOCK_TLV_L_LEN);
        }
        else if (AI_CLOCK_TLV_L_LEN == 2) {
            memcpy(&tmp_l2, buff + offset, AI_CLOCK_TLV_L_LEN);
        }
        else {
            TAL_PR_ERR("%s: unsupport TLV L len '%d' !", __func__, AI_CLOCK_TLV_L_LEN);
            return rt;
        }
        offset += AI_CLOCK_TLV_L_LEN;

        //parse 'V'
        if (AI_CLOCK_TLV_L_LEN == 1) {
            if (tmp_t1 == T) {
                rt = OPRT_OK;
                if (out_string) {
                    CHAR_T **p1 = (CHAR_T **)out_V;
                    *p1 = tal_malloc(tmp_l1 + 1);       //增加结束符空间!
                    if (*p1 == NULL) {
                        TAL_PR_ERR("%s: malloc fail for str '%s' !", __func__, (CHAR_T *)(buff + offset));
                        rt = OPRT_MALLOC_FAILED;
                    }
                    memset(*p1, 0, tmp_l1 + 1);
                    memcpy(*p1, buff + offset, tmp_l1);
                }
                else {
                    memcpy(out_V, buff + offset, tmp_l1);
                }
                break;
            }
            offset += tmp_l1;
        }
        else {
            if (tmp_t2 == T) {
                rt = OPRT_OK;
                if (out_string) {
                    CHAR_T **p2 = (CHAR_T **)out_V;
                    *p2 = tal_malloc(tmp_l2 + 1);       //增加结束符空间!
                    if (*p2 == NULL) {
                        TAL_PR_ERR("%s: malloc fail for str '%s' !", __func__, (CHAR_T *)(buff + offset));
                        rt = OPRT_MALLOC_FAILED;
                    }
                    memset(*p2, 0, tmp_l2 + 1);
                    memcpy(*p2, buff + offset, tmp_l2);
                }
                else {
                    memcpy(out_V, buff + offset, tmp_l2);
                }
                break;
            }
            offset += tmp_l2;
        }
    }

    return rt;
}

TIME_T wukong_clock_time_mktime(CHAR_T *iso_8601_time_str)
{
    CHAR_T * timr_str = NULL;
    POSIX_TM_S timer_posix_tm;
    INT_T time_zone = 0;
    TIME_T _time = 0;

    if (iso_8601_time_str == NULL)
        return _time;

    timr_str = mm_strdup(iso_8601_time_str);
    if (timr_str == NULL) {
        TAL_PR_ERR("malloc fail !");
        return _time;
    }

    if (sscanf(timr_str, "%d-%d-%dT%d:%d:%d",
               &timer_posix_tm.tm_year, &timer_posix_tm.tm_mon, &timer_posix_tm.tm_mday,
               &timer_posix_tm.tm_hour, &timer_posix_tm.tm_min, &timer_posix_tm.tm_sec) != 6) {
        TAL_PR_ERR("parse time '%s' fail !", timr_str);
        return _time;  // 解析失败
    }

    TAL_PR_DEBUG("trans time '%s' to %04d-%02d-%02d, %02d:%02d:%02d", iso_8601_time_str, 
                timer_posix_tm.tm_year, timer_posix_tm.tm_mon, timer_posix_tm.tm_mday,
                timer_posix_tm.tm_hour, timer_posix_tm.tm_min, timer_posix_tm.tm_sec);
    timer_posix_tm.tm_year -= 1900;
    timer_posix_tm.tm_mon -= 1;
    _time = tal_time_mktime(&timer_posix_tm);
    Free(timr_str);
    timr_str = NULL;
    tal_time_get_time_zone_seconds(&time_zone);
    _time -= time_zone;                         //扣除时区!
#if 0
{
    memset(&timer_posix_tm, 0, SIZEOF(timer_posix_tm));
    tal_time_get_local_time_custom(_time, &timer_posix_tm);
    TAL_PR_DEBUG("recovery time : %04d-%02d-%02d, %02d:%02d:%02d",
                timer_posix_tm.tm_year + 1900, timer_posix_tm.tm_mon + 1, timer_posix_tm.tm_mday,
                timer_posix_tm.tm_hour, timer_posix_tm.tm_min, timer_posix_tm.tm_sec);
}
#endif
    return _time;
}

/* 统一写入下一次 AI 播报的提示词，避免定时器回调再去拼接上下文。 */
STATIC VOID clock_alarm_speak_prompt_update(CONST CHAR_T *prompt)
{
    if ((prompt == NULL) || (prompt[0] == '\0')) {
        snprintf(__AlarmSpeakPrompt, AI_CLOCK_ALARM_PROMPT_MAX_LEN,
                 "System event: a local alarm has just fired. Speak one short, natural English reminder sentence. Feel free to vary the wording, but do not continue chatting.");
        return;
    }

    snprintf(__AlarmSpeakPrompt, AI_CLOCK_ALARM_PROMPT_MAX_LEN, "%s", prompt);
}

/* Delay the AI TTS a little so the local dingdong prompt can finish first. */
STATIC OPERATE_RET clock_alarm_speak_service_prepare(VOID)
{
    OPERATE_RET rt = OPRT_OK;

    if (__AlarmSpeakTimer == NULL) {
        TUYA_CALL_ERR_RETURN(tal_sw_timer_create(__wukong_clock_alarm_speak_timer_cb, NULL, &__AlarmSpeakTimer));
    }

    return rt;
}

/* 在本地提示音之后再拉起 AI 播报，并把当前提醒上下文一并带过去。 */
STATIC OPERATE_RET clock_alarm_speak_schedule(CONST CHAR_T *prompt)
{
    OPERATE_RET rt = OPRT_OK;

    TUYA_CALL_ERR_RETURN(clock_alarm_speak_service_prepare());
    clock_alarm_speak_prompt_update(prompt);
    (VOID)tal_sw_timer_stop(__AlarmSpeakTimer);
    TUYA_CALL_ERR_RETURN(tal_sw_timer_start(__AlarmSpeakTimer, AI_CLOCK_ALARM_SPEAK_DELAY_MS, TAL_TIMER_ONCE));
    return rt;
}

/* 触发一轮独立的 AI 文本播报，让模型基于上下文自然组织提醒话术。 */
STATIC VOID __wukong_clock_alarm_speak_timer_cb(TIMER_ID timer_id, VOID_T *arg)
{
    OPERATE_RET rt = OPRT_OK;

    (VOID)timer_id;
    (VOID)arg;

    rt = wukong_ai_agent_send_text(__AlarmSpeakPrompt);
    if (rt != OPRT_OK) {
        TAL_PR_ERR("%s: send alarm tts fail %d", __func__, rt);
    }
}

/* Reset the local countdown runtime state while keeping the timer reusable. */
STATIC VOID clock_countdown_state_reset(VOID)
{
    __CountdownDeadline = 0;
    __CountdownRemainSec = 0;
    __CountdownRunning = FALSE;
}

/* Prepare the local countdown timer so short voice reminders can complete without any screen module. */
STATIC OPERATE_RET clock_countdown_service_prepare(VOID)
{
    OPERATE_RET rt = OPRT_OK;

    if (__CountdownMutex == NULL) {
        TUYA_CALL_ERR_RETURN(tal_mutex_create_init(&__CountdownMutex));
    }

    if (__CountdownTimer == NULL) {
        TUYA_CALL_ERR_RETURN(tal_sw_timer_create(__wukong_clock_countdown_timer_cb, NULL, &__CountdownTimer));
    }

    return OPRT_OK;
}

/* Drive the local countdown lifecycle for start/pause/resume/stop operations. */
STATIC OPERATE_RET clock_countdown_control(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, INT_T total_seconds)
{
    OPERATE_RET rt = OPRT_OK;
    TIME_T now_time = 0;

    TUYA_CALL_ERR_RETURN(clock_countdown_service_prepare());

    tal_mutex_lock(__CountdownMutex);
    switch (opr) {
        case AI_CLOCK_TIMER_OPR_START:
            if (total_seconds <= 0) {
                rt = OPRT_INVALID_PARM;
                break;
            }
            (VOID)tal_sw_timer_stop(__CountdownTimer);
            __CountdownRemainSec = total_seconds;
            __CountdownDeadline = tal_time_get_posix() + total_seconds;
            __CountdownRunning = TRUE;
            rt = tal_sw_timer_start(__CountdownTimer, total_seconds * 1000, TAL_TIMER_ONCE);
            if (rt != OPRT_OK) {
                clock_countdown_state_reset();
            }
            break;
        case AI_CLOCK_TIMER_OPR_PAUSE:
            if (__CountdownRunning == FALSE) {
                break;
            }
            now_time = tal_time_get_posix();
            if (__CountdownDeadline > now_time) {
                __CountdownRemainSec = __CountdownDeadline - now_time;
            } else {
                __CountdownRemainSec = 1;
            }
            (VOID)tal_sw_timer_stop(__CountdownTimer);
            __CountdownDeadline = 0;
            __CountdownRunning = FALSE;
            break;
        case AI_CLOCK_TIMER_OPR_RESUME:
            if (__CountdownRemainSec <= 0) {
                rt = OPRT_INVALID_PARM;
                break;
            }
            now_time = tal_time_get_posix();
            (VOID)tal_sw_timer_stop(__CountdownTimer);
            __CountdownDeadline = now_time + __CountdownRemainSec;
            __CountdownRunning = TRUE;
            rt = tal_sw_timer_start(__CountdownTimer, __CountdownRemainSec * 1000, TAL_TIMER_ONCE);
            if (rt != OPRT_OK) {
                clock_countdown_state_reset();
            }
            break;
        case AI_CLOCK_TIMER_OPR_STOP:
        case AI_CLOCK_TIMER_OPR_RESET:
            (VOID)tal_sw_timer_stop(__CountdownTimer);
            clock_countdown_state_reset();
            break;
        default:
            rt = OPRT_INVALID_PARM;
            break;
    }
    tal_mutex_unlock(__CountdownMutex);

    return rt;
}

/* Fire the local countdown alert and clear state so the next reminder starts cleanly. */
STATIC VOID __wukong_clock_countdown_timer_cb(TIMER_ID timer_id, VOID_T *arg)
{
    OPERATE_RET rt = OPRT_OK;

    (VOID)timer_id;
    (VOID)arg;

    if (__CountdownMutex != NULL) {
        tal_mutex_lock(__CountdownMutex);
        clock_countdown_state_reset();
        tal_mutex_unlock(__CountdownMutex);
    }

    wukong_audio_player_alert(AI_TOY_ALERT_TYPE_WAKEUP, FALSE);
    // 本地先响一声，再让 AI 用英文自然提醒倒计时已经结束。
    rt = clock_alarm_speak_schedule("System event: the local countdown has finished. Speak one short, natural English reminder sentence. Feel free to vary the wording, but do not continue chatting.");
    if (rt != OPRT_OK) {
        TAL_PR_ERR("%s: schedule alarm speak fail %d", __func__, rt);
    }
}

OPERATE_RET wukong_clock_countdown_init(VOID)
{
    return clock_countdown_service_prepare();
}

OPERATE_RET wukong_clock_countdown_deinit(VOID)
{
    if (__CountdownTimer != NULL) {
        (VOID)tal_sw_timer_stop(__CountdownTimer);
        (VOID)tal_sw_timer_delete(__CountdownTimer);
        __CountdownTimer = NULL;
    }

    if (__CountdownMutex != NULL) {
        tal_mutex_release(__CountdownMutex);
        __CountdownMutex = NULL;
    }

    clock_countdown_state_reset();
    return OPRT_OK;
}

/* Create the internal alarm list and timer so MCP alarms can run without any UI module. */
STATIC OPERATE_RET clock_alarm_internal_service_prepare(VOID)
{
    OPERATE_RET rt = OPRT_OK;

    if (__AlarmList == NULL) {
        memset(&__AlarmListStorage, 0, SIZEOF(__AlarmListStorage));
        TUYA_CALL_ERR_RETURN(tal_mutex_create_init(&__AlarmListStorage.mutex));
        INIT_LIST_HEAD(&__AlarmListStorage.list_hd);
        __AlarmList = &__AlarmListStorage;
        __AlarmListOwned = TRUE;
    }

    if (__AlarmTimer == NULL) {
        TUYA_CALL_ERR_RETURN(tal_sw_timer_create(__wukong_clock_alarm_check_timer_cb, NULL, &__AlarmTimer));
        TUYA_CALL_ERR_RETURN(tal_sw_timer_start(__AlarmTimer, AI_CLOCK_ALARM_CHECK_PERIOD_MS, TAL_TIMER_CYCLE));
    }

    TUYA_CALL_ERR_RETURN(clock_alarm_speak_service_prepare());

    return rt;
}

/* Release one alarm node and its duplicated label string. */
STATIC VOID clock_alarm_element_free(TY_AI_CLOCK_ALARM_ELEMENT_S *alarm_ele)
{
    if (alarm_ele == NULL) {
        return;
    }

    if (alarm_ele->alarm_ele.label != NULL) {
        tal_free(alarm_ele->alarm_ele.label);
        alarm_ele->alarm_ele.label = NULL;
    }
    tal_free(alarm_ele);
}

/* Duplicate the alarm payload so the list can outlive the transient MCP property buffer. */
STATIC OPERATE_RET clock_alarm_element_fill(TY_AI_CLOCK_ALARM_ELEMENT_S *dst, CONST TY_AI_CLOCK_ALARM_CFG_T *src)
{
    if ((dst == NULL) || (src == NULL)) {
        return OPRT_INVALID_PARM;
    }

    memset(dst, 0, SIZEOF(*dst));
    memcpy(&dst->alarm_ele, src, SIZEOF(dst->alarm_ele));
    if ((src->label != NULL) && (strlen(src->label) > 0)) {
        dst->alarm_ele.label = mm_strdup(src->label);
        if (dst->alarm_ele.label == NULL) {
            TAL_PR_ERR("%s: duplicate label failed", __func__);
            return OPRT_MALLOC_FAILED;
        }
    } else {
        dst->alarm_ele.label = NULL;
    }

    return OPRT_OK;
}

/* Match an alarm slot so UPDATE can behave like an in-place replacement when the user refers to the same alarm. */
STATIC BOOL_T clock_alarm_slot_match(CONST TY_AI_CLOCK_ALARM_CFG_T *current, CONST TY_AI_CLOCK_ALARM_CFG_T *target)
{
    if ((current == NULL) || (target == NULL)) {
        return FALSE;
    }

    if (current->repeat_sched_freq != target->repeat_sched_freq) {
        return FALSE;
    }

    if ((current->detail_time > 0) || (target->detail_time > 0) ||
        (current->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE)) {
        if ((current->detail_time > 0) && (target->detail_time > 0)) {
            return (current->detail_time == target->detail_time);
        }
        return ((current->hours == target->hours) && (current->minutes == target->minutes));
    }

    if ((current->hours != target->hours) || (current->minutes != target->minutes)) {
        return FALSE;
    }

    if ((current->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_WEEKLY) &&
        (target->weekly_recurrence_day != 0) &&
        (current->weekly_recurrence_day != target->weekly_recurrence_day)) {
        return FALSE;
    }

    return TRUE;
}

/* Keep DELETE behavior aligned with the reference demo while still narrowing weekly deletes when weekday bits are present. */
STATIC BOOL_T clock_alarm_delete_match(CONST TY_AI_CLOCK_ALARM_CFG_T *current, CONST TY_AI_CLOCK_ALARM_CFG_T *target, BOOL_T delete_all)
{
    if ((current == NULL) || (target == NULL)) {
        return FALSE;
    }

    if (delete_all == TRUE) {
        return TRUE;
    }

    if ((target->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE) || (target->detail_time > 0)) {
        if ((target->detail_time > 0) && (current->detail_time > 0)) {
            return (current->detail_time == target->detail_time);
        }
        return ((current->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE) &&
                (current->hours == target->hours) &&
                (current->minutes == target->minutes));
    }

    if (target->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_DAILY) {
        return ((current->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_DAILY) &&
                (current->hours == target->hours) &&
                (current->minutes == target->minutes));
    }

    if (target->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_WEEKLY) {
        if ((current->repeat_sched_freq != AI_CLOCK_ALARM_RETEAT_WEEKLY) ||
            (current->hours != target->hours) ||
            (current->minutes != target->minutes)) {
            return FALSE;
        }
        if (target->weekly_recurrence_day == 0) {
            return TRUE;
        }
        return (current->weekly_recurrence_day == target->weekly_recurrence_day);
    }

    return clock_alarm_slot_match(current, target);
}

/* Calculate one alarm's trigger slot so one-shot alarms can honor exact seconds while repeats still dedupe by minute. */
STATIC TIME_T clock_alarm_trigger_slot_get(CONST TY_AI_CLOCK_ALARM_CFG_T *alarm, TIME_T cur_time_s,
                                           TIME_T current_minute_start, CONST POSIX_TM_S *local_tm)
{
    UCHAR_T week_day_bit = 0;

    if ((alarm == NULL) || (local_tm == NULL)) {
        return 0;
    }

    if (alarm->detail_time > 0) {
        if ((cur_time_s >= alarm->detail_time) && (cur_time_s < (alarm->detail_time + 60))) {
            return alarm->detail_time;
        }
        return 0;
    }

    if (alarm->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE) {
        if ((alarm->hours == local_tm->tm_hour) && (alarm->minutes == local_tm->tm_min)) {
            return current_minute_start;
        }
        return 0;
    }

    if ((alarm->hours != local_tm->tm_hour) || (alarm->minutes != local_tm->tm_min)) {
        return 0;
    }

    if (alarm->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_DAILY) {
        return current_minute_start;
    }

    if (alarm->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_WEEKLY) {
        week_day_bit = (1 << local_tm->tm_wday);
        if ((alarm->weekly_recurrence_day & week_day_bit) != 0) {
            return current_minute_start;
        }
    }

    return 0;
}

/* The periodic checker is intentionally lightweight: it only plays the built-in wakeup alert and clears one-shot alarms. */
STATIC VOID __wukong_clock_alarm_check_timer_cb(TIMER_ID timer_id, VOID_T *arg)
{
    BOOL_T need_alert = FALSE;
    OPERATE_RET rt = OPRT_OK;
    TIME_T cur_time_s = 0;
    TIME_T current_minute_start = 0;
    TIME_T trigger_slot = 0;
    CHAR_T speak_prompt[AI_CLOCK_ALARM_PROMPT_MAX_LEN] = {0};
    POSIX_TM_S local_tm;
    struct tuya_list_head *p = NULL;
    struct tuya_list_head *n = NULL;
    TY_AI_CLOCK_ALARM_ELEMENT_S *entry = NULL;

    (VOID)timer_id;
    (VOID)arg;

    if ((__AlarmList == NULL) || (__AlarmList->mutex == NULL)) {
        return;
    }

    cur_time_s = tal_time_get_posix();
    memset(&local_tm, 0, SIZEOF(local_tm));
    tal_time_get_local_time_custom(cur_time_s, &local_tm);
    current_minute_start = cur_time_s - local_tm.tm_sec;

    tal_mutex_lock(__AlarmList->mutex);
    tuya_list_for_each_safe(p, n, &__AlarmList->list_hd) {
        entry = tuya_list_entry(p, TY_AI_CLOCK_ALARM_ELEMENT_S, node);
        if (entry == NULL) {
            continue;
        }

        trigger_slot = clock_alarm_trigger_slot_get(&entry->alarm_ele, cur_time_s, current_minute_start, &local_tm);
        if ((trigger_slot > 0) && (entry->last_trigger_slot != trigger_slot)) {
            entry->last_trigger_slot = trigger_slot;
            need_alert = TRUE;
            // 把时间和标签带给 AI，方便它围绕当前闹钟生成英文提醒。
            if ((entry->alarm_ele.label != NULL) && (entry->alarm_ele.label[0] != '\0')) {
                snprintf(speak_prompt, SIZEOF(speak_prompt),
                         "System event: a local alarm has fired at %02d:%02d. The reminder label is \"%s\". Speak one short, natural English reminder sentence. If the label is not English, keep the reminder itself in English. Feel free to vary the wording, but do not continue chatting.",
                         entry->alarm_ele.hours, entry->alarm_ele.minutes, entry->alarm_ele.label);
            } else {
                snprintf(speak_prompt, SIZEOF(speak_prompt),
                         "System event: a local alarm has fired at %02d:%02d. Speak one short, natural English reminder sentence. Feel free to vary the wording, but do not continue chatting.",
                         entry->alarm_ele.hours, entry->alarm_ele.minutes);
            }
            TAL_PR_NOTICE("alarm triggered: %02d:%02d label=%s",
                          entry->alarm_ele.hours, entry->alarm_ele.minutes,
                          (entry->alarm_ele.label != NULL) ? entry->alarm_ele.label : "empty");
            if ((entry->alarm_ele.detail_time > 0) ||
                (entry->alarm_ele.repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE)) {
                tuya_list_del(&entry->node);
                clock_alarm_element_free(entry);
                entry = NULL;
            }
        }
    }
    tal_mutex_unlock(__AlarmList->mutex);

    if (need_alert == TRUE) {
        wukong_audio_player_alert(AI_TOY_ALERT_TYPE_WAKEUP, FALSE);
        // 闹钟先播本地提示音，再把具体时间和标签交给 AI 做自然播报。
        rt = clock_alarm_speak_schedule(speak_prompt);
        if (rt != OPRT_OK) {
            TAL_PR_ERR("%s: schedule alarm speak fail %d", __func__, rt);
        }
    }
}

OPERATE_RET wukong_clock_alarm_init(VOID)
{
    return clock_alarm_internal_service_prepare();
}

OPERATE_RET wukong_clock_alarm_deinit(VOID)
{
    struct tuya_list_head *p = NULL;
    struct tuya_list_head *n = NULL;
    TY_AI_CLOCK_ALARM_ELEMENT_S *entry = NULL;

    if (__AlarmTimer != NULL) {
        tal_sw_timer_stop(__AlarmTimer);
        tal_sw_timer_delete(__AlarmTimer);
        __AlarmTimer = NULL;
    }

    if (__AlarmSpeakTimer != NULL) {
        tal_sw_timer_stop(__AlarmSpeakTimer);
        tal_sw_timer_delete(__AlarmSpeakTimer);
        __AlarmSpeakTimer = NULL;
    }

    if ((__AlarmListOwned == TRUE) && (__AlarmList != NULL) && (__AlarmList->mutex != NULL)) {
        tal_mutex_lock(__AlarmList->mutex);
        tuya_list_for_each_safe(p, n, &__AlarmList->list_hd) {
            entry = tuya_list_entry(p, TY_AI_CLOCK_ALARM_ELEMENT_S, node);
            if (entry != NULL) {
                tuya_list_del(&entry->node);
                clock_alarm_element_free(entry);
            }
        }
        tal_mutex_unlock(__AlarmList->mutex);
        tal_mutex_release(__AlarmList->mutex);
        memset(&__AlarmListStorage, 0, SIZEOF(__AlarmListStorage));
    }

    __AlarmList = NULL;
    __AlarmListOwned = FALSE;
    return OPRT_OK;
}

OPERATE_RET wukong_clock_set_alarm(TY_AI_CLOCK_ALARM_OPR_TYPE_E opr, BOOL_T delete_all, TY_AI_CLOCK_ALARM_CFG_T *alarm)
{
    OPERATE_RET rt = OPRT_OK;
    struct tuya_list_head *p = NULL;
    struct tuya_list_head *n = NULL;
    TY_AI_CLOCK_ALARM_ELEMENT_S *entry = NULL;
    TY_AI_CLOCK_ALARM_ELEMENT_S *new_alarm = NULL;

    if (alarm == NULL) {
        TAL_PR_ERR("%s: alarm is null", __func__);
        return OPRT_INVALID_PARM;
    }

    if (((opr == AI_CLOCK_ALARM_OPR_ADD) || (opr == AI_CLOCK_ALARM_OPR_UPDATE)) &&
        (alarm->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE) &&
        (alarm->detail_time == 0)) {
        TAL_PR_ERR("%s: detail time is required for one-shot alarms", __func__);
        return OPRT_INVALID_PARM;
    }

    TUYA_CALL_ERR_RETURN(wukong_clock_alarm_init());

    if ((opr == AI_CLOCK_ALARM_OPR_ADD) || (opr == AI_CLOCK_ALARM_OPR_UPDATE)) {
        new_alarm = (TY_AI_CLOCK_ALARM_ELEMENT_S *)tal_malloc(SIZEOF(TY_AI_CLOCK_ALARM_ELEMENT_S));
        if (new_alarm == NULL) {
            TAL_PR_ERR("%s: malloc failed", __func__);
            return OPRT_MALLOC_FAILED;
        }
        rt = clock_alarm_element_fill(new_alarm, alarm);
        if (rt != OPRT_OK) {
            clock_alarm_element_free(new_alarm);
            return rt;
        }
    }

    tal_mutex_lock(__AlarmList->mutex);
    if ((opr == AI_CLOCK_ALARM_OPR_DELETE) || (opr == AI_CLOCK_ALARM_OPR_UPDATE)) {
        tuya_list_for_each_safe(p, n, &__AlarmList->list_hd) {
            entry = tuya_list_entry(p, TY_AI_CLOCK_ALARM_ELEMENT_S, node);
            if ((entry != NULL) &&
                ((opr == AI_CLOCK_ALARM_OPR_DELETE) ?
                 clock_alarm_delete_match(&entry->alarm_ele, alarm, delete_all) :
                 clock_alarm_slot_match(&entry->alarm_ele, alarm))) {
                tuya_list_del(&entry->node);
                clock_alarm_element_free(entry);
            }
        }
    }

    if ((opr == AI_CLOCK_ALARM_OPR_ADD) || (opr == AI_CLOCK_ALARM_OPR_UPDATE)) {
        tuya_list_add_tail(&new_alarm->node, &__AlarmList->list_hd);
    } else if (opr != AI_CLOCK_ALARM_OPR_DELETE) {
        TAL_PR_ERR("%s: unknown alarm operation %d", __func__, opr);
        rt = OPRT_INVALID_PARM;
    }
    tal_mutex_unlock(__AlarmList->mutex);

    if ((rt != OPRT_OK) && (new_alarm != NULL) &&
        ((opr == AI_CLOCK_ALARM_OPR_ADD) || (opr == AI_CLOCK_ALARM_OPR_UPDATE))) {
        clock_alarm_element_free(new_alarm);
    }

    return rt;
}

OPERATE_RET wukong_clock_set_countdown_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, INT_T hours, INT_T minutes, INT_T seconds)
{
    OPERATE_RET rt = OPRT_OK;
    INT_T total_seconds = 0;
    UINT_T offset = 0;
    UINT8_T *msg = NULL;
    INT_T len = 0;

    // 在无屏幕场景下，本地倒计时也需要独立完成到点提醒。
    total_seconds = hours * 3600 + minutes * 60 + seconds;
    TUYA_CALL_ERR_RETURN(clock_countdown_control(opr, total_seconds));

    TAL_PR_DEBUG("set operation to %d", opr);           // 1byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    TAL_PR_DEBUG("set hour_duration to %d", hours);     // 1byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    TAL_PR_DEBUG("set minute_duration to %d", minutes); // 1byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    TAL_PR_DEBUG("set second_duration to %d", seconds); // 1byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    msg = tal_malloc(len);
    if (msg == NULL) {
        TAL_PR_ERR("%s: malloc failed !", __func__);
        return OPRT_MALLOC_FAILED;
    }
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_COUNTDOWN_TAG_OPR,     1, (UINT8_T *)&opr,        &offset);              //operate
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_COUNTDOWN_TAG_HOUR,    1, (UINT8_T *)&hours,      &offset);              //hours
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_COUNTDOWN_TAG_MINUTE,  1, (UINT8_T *)&minutes,    &offset);              //minutes
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_COUNTDOWN_TAG_SECOND,  1, (UINT8_T *)&seconds,    &offset);              //seconds
    wukong_clock_tlv_data_packed_show(msg, len);

    wukong_ai_event_notify(WUKONG_AI_EVENT_CLOCK_MCP_COUNTDOWN_TIMER, msg);
    // tuya_ai_display_msg(msg, len, TY_DISPLAY_TP_CLOCK_MCP_COUNTDOWN_TIMER);
    tal_free(msg);
    msg = NULL;

    return OPRT_OK;
}

OPERATE_RET wukong_clock_set_stopwatch_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr)
{
    UINT_T offset = 0;
    UINT8_T *msg = NULL;
    INT_T len = 0;

    TAL_PR_DEBUG("set operation to %d", opr);           // 1 byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    msg = tal_malloc(len);
    if (msg == NULL) {
        TAL_PR_ERR("%s: malloc failed !", __func__);
        return OPRT_MALLOC_FAILED;
    }

    wukong_clock_tlv_data_pack(msg, AI_CLOCK_STOPWATCH_TAG_OPR,     1, (UINT8_T *)&opr,        &offset);              //operate
    wukong_clock_tlv_data_packed_show(msg, len);

    wukong_ai_event_notify(WUKONG_AI_EVENT_CLOCK_MCP_STOPWATCH_TIMER, msg);
    // tuya_ai_display_msg(msg, len, TY_DISPLAY_TP_CLOCK_MCP_STOPWATCH_TIMER);
    tal_free(msg);
    msg = NULL;

    return OPRT_OK;
}

OPERATE_RET wukong_clock_set_pomodoro_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, TY_AI_CLOCK_POMODORO_TIMER_CFG_T *pomodoro)
{
    UINT_T offset = 0;
    UINT8_T *msg = NULL;
    INT_T len = 0;

    TAL_PR_DEBUG("set operation to %d", opr);                                       // 1 byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    TAL_PR_DEBUG("set work_duration to %d", pomodoro->work_duration);               // 1 byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    TAL_PR_DEBUG("set short_break_duration to %d", pomodoro->short_break_duration); // 1 byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    TAL_PR_DEBUG("set long_break_duration to %d", pomodoro->long_break_duration);   // 1 byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);
    msg = tal_malloc(len);
    if (msg == NULL) {
        TAL_PR_ERR("%s: malloc failed !", __func__);
        return OPRT_MALLOC_FAILED;
    }

    wukong_clock_tlv_data_pack(msg, AI_CLOCK_POMODORO_TAG_OPR,          1, (UINT8_T *)&opr,                            &offset);              //operate
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_POMODORO_TAG_WORK_DUR,     1, (UINT8_T *)&pomodoro->work_duration,        &offset);              //work_duration
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_POMODORO_TAG_SHORT_BRK_DUR,1, (UINT8_T *)&pomodoro->short_break_duration, &offset);              //short_break_duration
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_POMODORO_TAG_LONG_BRK_DUR, 1, (UINT8_T *)&pomodoro->long_break_duration,  &offset);              //long_break_duration
    wukong_clock_tlv_data_packed_show(msg, len);

    wukong_ai_event_notify(WUKONG_AI_EVENT_CLOCK_MCP_POMODORO_TIMER, msg);
    // tuya_ai_display_msg(msg, len, TY_DISPLAY_TP_CLOCK_MCP_POMODORO_TIMER);
    tal_free(msg);
    msg = NULL;

    return OPRT_OK;
}

OPERATE_RET wukong_clock_set_schedule(TY_AI_CLOCK_SCHED_OPR_TYPE_E opr, TY_AI_CLOCK_SCHED_CFG_T *sched)
{
    UINT_T offset = 0;
    UINT8_T *msg = NULL;
    INT_T len = 0;

    TAL_PR_DEBUG("set operation to %d", opr);                       // 1 byte
    len += (AI_CLOCK_TLV_TL_LEN + 1);

    if (sched->start_time > 0) {
        TAL_PR_DEBUG("set start_time to %d", sched->start_time);    // x byte
        len += (AI_CLOCK_TLV_TL_LEN + SIZEOF(sched->start_time));
    }
    if (sched->end_time > 0) {
        TAL_PR_DEBUG("set end_time to %d", sched->end_time);        // x byte
        len += (AI_CLOCK_TLV_TL_LEN + SIZEOF(sched->end_time));
    }
    if (sched->location && strlen(sched->location) > 0) {
        TAL_PR_DEBUG("set location to %s", sched->location);        // x byte
        len += (AI_CLOCK_TLV_TL_LEN + strlen(sched->location));
    }
    if (sched->description && strlen(sched->description) > 0) {
        TAL_PR_DEBUG("set description to %s", sched->description);  // x byte
        len += (AI_CLOCK_TLV_TL_LEN + strlen(sched->description));
    }

    TAL_PR_DEBUG("set categories to %d", sched->categories);        // x byte
    len += (AI_CLOCK_TLV_TL_LEN + SIZEOF(sched->categories));

    msg = tal_malloc(len);
    if (msg == NULL) {
        TAL_PR_ERR("%s: malloc failed !", __func__);
        return OPRT_MALLOC_FAILED;
    }

    wukong_clock_tlv_data_pack(msg, AI_CLOCK_SCHED_TAG_OPR,           1,                         (UINT8_T *)&opr,                 &offset);              //operate
    if (sched->start_time > 0) {
        wukong_clock_tlv_data_pack(msg, AI_CLOCK_SCHED_TAG_START_TIME,SIZEOF(sched->start_time), (UINT8_T *)&sched->start_time,   &offset);              //start_time
    }
    if (sched->end_time > 0) {
        wukong_clock_tlv_data_pack(msg, AI_CLOCK_SCHED_TAG_END_TIME,  SIZEOF(sched->end_time),   (UINT8_T *)&sched->end_time,     &offset);              //end_time
    }
    if (sched->location && strlen(sched->location) > 0) {
        wukong_clock_tlv_data_pack(msg, AI_CLOCK_SCHED_TAG_LOCATION,  strlen(sched->location),   (UINT8_T *)(sched->location),    &offset);              //location
    }
    if (sched->description && strlen(sched->description) > 0) {
        wukong_clock_tlv_data_pack(msg, AI_CLOCK_SCHED_TAG_DESC,      strlen(sched->description),(UINT8_T *)(sched->description), &offset);              //description
    }
    wukong_clock_tlv_data_pack(msg, AI_CLOCK_SCHED_TAG_CATEGORY,      SIZEOF(sched->categories), (UINT8_T *)&sched->categories,   &offset);              //category
    wukong_clock_tlv_data_packed_show(msg, len);

    wukong_ai_event_notify(WUKONG_AI_EVENT_CLOCK_MCP_SCHEDULE, msg);
    // tuya_ai_display_msg(msg, len, TY_DISPLAY_TP_CLOCK_MCP_SCHEDULE);
    tal_free(msg);
    msg = NULL;

    return OPRT_OK;
}

STATIC CHAR_T *clock_schedule_query_by_query_method(TY_AI_CLOCK_SCHED_QUERY_METHOD_E query_method, TY_AI_CLOCK_SCHED_QUERY_CFG_T *sched_query)
{
    //input categorie:(0=meeting, 1=work, 2=personal, 3=health, 4=learning, 5=social, 6=other)
    CHAR_T *_content = NULL;
    UINT_T offset = 0;
    UCHAR_T total = 0;
    INT_T len = 0;
    struct tuya_list_head *p = NULL;
    struct tuya_list_head *n = NULL;
    TY_AI_CLOCK_SCHED_ELEMENT_S *entry = NULL;
    //query info
    TIME_T start_time = sched_query->start_time; 
    TIME_T end_time = sched_query->end_time;
    TY_AI_CLOCK_SCHED_CATEGORY_E categorie = sched_query->categories;
    CHAR_T *keyword = sched_query->keyword;

    if (__SchedList) {
        tal_mutex_lock(__SchedList->mutex);
        tuya_list_for_each_safe(p, n, &__SchedList->list_hd) {
            entry = tuya_list_entry(p, TY_AI_CLOCK_SCHED_ELEMENT_S, node);
            if (NULL != entry &&    \
                (((query_method == AI_CLOCK_SCHED_QUERY_BY_TIME) && (entry->sched_ele.start_time >= start_time) && (entry->sched_ele.end_time <= end_time)) ||    \
                ((query_method == AI_CLOCK_SCHED_QUERY_BY_CATEGORY) && (entry->sched_ele.categories == categorie)) ||   \
                ((query_method == AI_CLOCK_SCHED_QUERY_BY_KEYWORD) && (strstr(entry->sched_ele.location, keyword) != NULL || strstr(entry->sched_ele.description, keyword) != NULL)))) {
                total++;
                len += 20;      //start time, such as: 2025-12-23T16:00:00
                len += 4;       //string : ' to '
                len += 20;      //end time, such as: 2025-12-23T17:00:00
                if (entry->sched_ele.location != NULL)
                    len += (strlen(entry->sched_ele.location) + 1);
                if (entry->sched_ele.description != NULL)
                    len += (strlen(entry->sched_ele.description) + 1);
                len += 3;       //line separator: '\n'
            }
        }

        //Restructure contents:
        if (total > 0) {
            len += 64;          //total descrition line : 'Query Results: Found x schedules\n'
            _content = (CHAR_T *)tal_malloc(len);
            if (_content == NULL) {
                TAL_PR_ERR("%s: malloc failed !", __func__);
            }
            else {
                POSIX_TM_S timer_posix_tm;
                p = NULL;
                n = NULL;
                memset(_content, 0, len);
                offset += snprintf(_content+offset, len-offset, "Query Results: Found %d schedules\n", total);
                tuya_list_for_each_safe(p, n, &__SchedList->list_hd) {
                    entry = tuya_list_entry(p, TY_AI_CLOCK_SCHED_ELEMENT_S, node);
                    if (NULL != entry &&    \
                        (((query_method == AI_CLOCK_SCHED_QUERY_BY_TIME) && (entry->sched_ele.start_time >= start_time) && (entry->sched_ele.end_time <= end_time)) ||    \
                        ((query_method == AI_CLOCK_SCHED_QUERY_BY_CATEGORY) && (entry->sched_ele.categories == categorie)) ||   \
                        ((query_method == AI_CLOCK_SCHED_QUERY_BY_KEYWORD) && (strstr(entry->sched_ele.location, keyword) != NULL || strstr(entry->sched_ele.description, keyword) != NULL)))) {
                        memset(&timer_posix_tm, 0, SIZEOF(timer_posix_tm));
                        tal_time_get_local_time_custom(entry->sched_ele.start_time, &timer_posix_tm);
                        offset += snprintf(_content+offset, len-offset, "%04d-%02d-%02dT%02d:%02d:%02d", 
                                            timer_posix_tm.tm_year + 1900, timer_posix_tm.tm_mon + 1, timer_posix_tm.tm_mday,
                                            timer_posix_tm.tm_hour, timer_posix_tm.tm_min, timer_posix_tm.tm_sec);
                        offset += snprintf(_content+offset, len-offset, " %s ", "to");
                        memset(&timer_posix_tm, 0, SIZEOF(timer_posix_tm));
                        tal_time_get_local_time_custom(entry->sched_ele.end_time, &timer_posix_tm);
                        offset += snprintf(_content+offset, len-offset, "%04d-%02d-%02dT%02d:%02d:%02d", 
                                            timer_posix_tm.tm_year + 1900, timer_posix_tm.tm_mon + 1, timer_posix_tm.tm_mday,
                                            timer_posix_tm.tm_hour, timer_posix_tm.tm_min, timer_posix_tm.tm_sec);
                        offset += snprintf(_content+offset, len-offset, " %s", entry->sched_ele.description);
                        if (entry->sched_ele.location != NULL)
                            offset += snprintf(_content+offset, len-offset, "(%s)", entry->sched_ele.location);
                        offset += snprintf(_content+offset, len-offset, "\n");
                    }
                }
            }
        }
        tal_mutex_unlock(__SchedList->mutex);
    }

    return _content;
}

STATIC INT_T alarm_bit_to_weekday(UCHAR_T bit_mask)
{
    STATIC CONST INT_T bit2weekday[] = {7, 1, 2, 3, 4, 5, 6};
    UCHAR_T idx = 0;
    UCHAR_T mask = bit_mask;

    while (((mask & 1) == 0) && (idx < SIZEOF(bit2weekday) / SIZEOF(bit2weekday[0]))) {
        mask >>= 1;
        idx++;
    }

    return bit2weekday[(idx >= (SIZEOF(bit2weekday) / SIZEOF(bit2weekday[0]))) ? 0 : idx];
}

/* Format one alarm occurrence into the LLM-facing query string. */
STATIC OPERATE_RET alarm_query_content_node_generate(TY_AI_CLOCK_ALARM_CFG_T *alarm, TIME_T clock_time,
                                                     CHAR_T **buf, UINT_T *len, UINT_T *offset)
{
    OPERATE_RET ret = OPRT_OK;
    POSIX_TM_S tm;
    UINT_T left_len = 0;
    UINT_T label_len = 0;

    memset(&tm, 0, SIZEOF(tm));
    tal_time_get_local_time_custom(clock_time, &tm);
    if (alarm->label != NULL) {
        label_len += (16 + strlen(alarm->label));
    }

    left_len = *len - *offset;
    if (left_len < (20 + label_len)) {
        *len += ALARM_MALLOC_SIZE;
        *buf = tal_realloc(*buf, *len);
        if (*buf == NULL) {
            TAL_PR_ERR("%s-%d: realloc buff len '%d' fail !", __func__, __LINE__, *len);
            return OPRT_MALLOC_FAILED;
        }
        memset(*buf + *offset, 0, *len - *offset);
    }

    *offset += snprintf(*buf + *offset, *len - *offset, "%04d-%02d-%02dT%02d:%02d:%02d",
                        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
    if (alarm->label != NULL) {
        *offset += snprintf(*buf + *offset, *len - *offset, "(alarm label:%s)", alarm->label);
    }
    *offset += snprintf(*buf + *offset, *len - *offset, "\n");

    return ret;
}

/* Expand one logical alarm into the occurrences that fall into the requested range. */
STATIC CHAR_T *alarm_query_content_generate(TY_AI_CLOCK_ALARM_CFG_T *alarm, TIME_T start_time, TIME_T end_time,
                                            TY_AI_CLOCK_ALARM_QUERY_METHOD_E query_method, CHAR_T **alarm_content,
                                            UINT_T *offset, UINT_T *total, UINT_T *len)
{
    OPERATE_RET ret = OPRT_OK;
    INT_T time_zone = 0;
    TIME_T clock_time = 0;
    TIME_T now_time = 0;
    POSIX_TM_S current_tm;
    POSIX_TM_S tmp_tm;
    INT_T current_weekday = 0;
    INT_T target_weekday = 0;
    INT_T days_to_add = 0;

    if (*alarm_content == NULL) {
        *len = ALARM_MALLOC_SIZE;
        *alarm_content = (CHAR_T *)tal_malloc(*len);
        if (*alarm_content == NULL) {
            TAL_PR_ERR("%s: malloc failed !", __func__);
            return NULL;
        }
        memset(*alarm_content, 0, *len);
        *offset += strlen(ALARM_CONTENT_HEAD);
    }

    now_time = tal_time_get_posix();
    tal_time_get_local_time_custom(now_time, &current_tm);
    tal_time_get_time_zone_seconds(&time_zone);

    if (alarm->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_ONCE) {
        if (alarm->detail_time == 0) {
            TAL_PR_ERR("%s: miss detail time info of one time alarm [hour:%d, minute:%d]!",
                       __func__, alarm->hours, alarm->minutes);
        } else if (((query_method == AI_CLOCK_ALARM_QUERY_BY_TIME) &&
                    (alarm->detail_time >= start_time) &&
                    (alarm->detail_time <= end_time)) ||
                   (query_method == AI_CLOCK_ALARM_QUERY_ALL)) {
            *total += 1;
            clock_time = alarm->detail_time;
            ret = alarm_query_content_node_generate(alarm, clock_time, alarm_content, len, offset);
            if (ret != OPRT_OK) {
                *total = 0;
            }
        }
    } else if (alarm->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_DAILY) {
        for (INT_T i = 0;; i++) {
            memcpy(&tmp_tm, &current_tm, SIZEOF(POSIX_TM_S));
            tmp_tm.tm_hour = alarm->hours;
            tmp_tm.tm_min = alarm->minutes;
            tmp_tm.tm_sec = 0;
            tmp_tm.tm_mday += i;
            clock_time = tal_time_mktime(&tmp_tm);
            clock_time -= time_zone;
            if (((query_method == AI_CLOCK_ALARM_QUERY_BY_TIME) &&
                 (clock_time >= start_time) &&
                 (clock_time <= end_time)) ||
                (query_method == AI_CLOCK_ALARM_QUERY_ALL)) {
                *total += 1;
                ret = alarm_query_content_node_generate(alarm, clock_time, alarm_content, len, offset);
                if (ret != OPRT_OK) {
                    *total = 0;
                    break;
                } else if (*total >= ALARM_MAX_NUM) {
                    break;
                }
            } else if (clock_time > end_time) {
                break;
            }
        }
    } else if (alarm->repeat_sched_freq == AI_CLOCK_ALARM_RETEAT_WEEKLY) {
        BOOL_T time_exceed = FALSE;

        current_weekday = current_tm.tm_wday;
        for (INT_T cycle_cnt = 0;; cycle_cnt++) {
            for (INT_T i = 0; i < 7; i++) {
                if ((alarm->weekly_recurrence_day & (1 << i)) == 0) {
                    continue;
                }

                target_weekday = alarm_bit_to_weekday(alarm->weekly_recurrence_day & (1 << i));
                if (target_weekday < current_weekday) {
                    days_to_add = 7 - (current_weekday - target_weekday);
                } else {
                    days_to_add = target_weekday - current_weekday;
                }

                memcpy(&tmp_tm, &current_tm, SIZEOF(POSIX_TM_S));
                tmp_tm.tm_hour = alarm->hours;
                tmp_tm.tm_min = alarm->minutes;
                tmp_tm.tm_sec = 0;
                tmp_tm.tm_mday += (days_to_add + cycle_cnt * 7);
                clock_time = tal_time_mktime(&tmp_tm);
                clock_time -= time_zone;
                if (((query_method == AI_CLOCK_ALARM_QUERY_BY_TIME) &&
                     (clock_time >= start_time) &&
                     (clock_time <= end_time)) ||
                    (query_method == AI_CLOCK_ALARM_QUERY_ALL)) {
                    *total += 1;
                    ret = alarm_query_content_node_generate(alarm, clock_time, alarm_content, len, offset);
                    if (ret != OPRT_OK) {
                        *total = 0;
                        time_exceed = TRUE;
                        break;
                    } else if (*total >= ALARM_MAX_NUM) {
                        time_exceed = TRUE;
                        break;
                    }
                } else if (clock_time > end_time) {
                    time_exceed = TRUE;
                }
            }
            if (time_exceed == TRUE) {
                break;
            }
        }
    }

    if (ret != OPRT_OK) {
        if (*alarm_content != NULL) {
            tal_free(*alarm_content);
        }
        *alarm_content = NULL;
    }

    return *alarm_content;
}

/* Query alarms from the local in-memory list so MCP can answer even when no display layer is compiled in. */
STATIC CHAR_T *clock_alarm_query_by_query_method(TY_AI_CLOCK_ALARM_QUERY_METHOD_E query_method,
                                                 TY_AI_CLOCK_ALARM_QUERY_CFG_T *alarm_query)
{
    struct tuya_list_head *p = NULL;
    struct tuya_list_head *n = NULL;
    TY_AI_CLOCK_ALARM_ELEMENT_S *entry = NULL;
    TIME_T start_time = alarm_query->start_time;
    TIME_T end_time = alarm_query->end_time;
    CHAR_T *content_hd = NULL;
    CHAR_T *alarm_content = NULL;
    UINT_T offset = 0;
    UINT_T total = 0;
    UINT_T len = 0;

    if (__AlarmList == NULL) {
        return NULL;
    }

    tal_mutex_lock(__AlarmList->mutex);
    tuya_list_for_each_safe(p, n, &__AlarmList->list_hd) {
        entry = tuya_list_entry(p, TY_AI_CLOCK_ALARM_ELEMENT_S, node);
        if (entry == NULL) {
            continue;
        }
        alarm_query_content_generate(&entry->alarm_ele, start_time, end_time, query_method,
                                     &alarm_content, &offset, &total, &len);
        if (alarm_content == NULL) {
            break;
        } else if (total >= ALARM_MAX_NUM) {
            break;
        }
    }
    if (alarm_content != NULL) {
        content_hd = (CHAR_T *)tal_malloc(strlen(ALARM_CONTENT_HEAD) + 1);
        if (content_hd != NULL) {
            memset(content_hd, 0, strlen(ALARM_CONTENT_HEAD) + 1);
            snprintf(content_hd, strlen(ALARM_CONTENT_HEAD) + 1, "Query Results: Found %03d alarms\n", total);
            memcpy(alarm_content, content_hd, strlen(ALARM_CONTENT_HEAD));
            tal_free(content_hd);
        } else {
            TAL_PR_ERR("%s: malloc failed !", __func__);
            tal_free(alarm_content);
            alarm_content = NULL;
        }
    }
    tal_mutex_unlock(__AlarmList->mutex);

    return alarm_content;
}

CHAR_T *wukong_clock_set_schedule_query(TY_AI_CLOCK_SCHED_QUERY_METHOD_E query_method, TY_AI_CLOCK_SCHED_QUERY_CFG_T *sched_query)
{
    CHAR_T *content = NULL;

    TAL_PR_DEBUG("set schedule query_method to %d", query_method);

    if (query_method == AI_CLOCK_SCHED_QUERY_BY_TIME) {
        if (sched_query->start_time <= 0 || sched_query->end_time <= 0) {
            TAL_PR_ERR("%s: miss time info when query schedule by time !", __func__);
            return NULL;
        }
        TAL_PR_DEBUG("set schedule query time from '%d' to %d", sched_query->start_time, sched_query->end_time);
    }
    else if (query_method == AI_CLOCK_SCHED_QUERY_BY_CATEGORY) {
        TAL_PR_DEBUG("set schedule query categories as %d", sched_query->categories);
    }
    else if (query_method == AI_CLOCK_SCHED_QUERY_BY_KEYWORD) {
        if (sched_query->keyword == NULL) {
            TAL_PR_ERR("%s: miss keyword info when query schedule by keyword !", __func__);
            return NULL;
        }
        TAL_PR_DEBUG("set schedule query keyword as '%s'", sched_query->keyword);
    }
    else {
        TAL_PR_ERR("%s: unknown query method '%d' !", __func__, query_method);
        return NULL;
    }

    content = clock_schedule_query_by_query_method(query_method, sched_query);
    if (content != NULL) {
        TAL_PR_DEBUG("%s. query content: '%s' !", __func__, content);
    }
    return content;
}

CHAR_T *wukong_clock_set_alarm_query(TY_AI_CLOCK_ALARM_QUERY_METHOD_E query_method, TY_AI_CLOCK_ALARM_QUERY_CFG_T *alarm_query)
{
    CHAR_T *content = NULL;

    if (alarm_query == NULL) {
        TAL_PR_ERR("%s: alarm_query is null", __func__);
        return NULL;
    }

    if (wukong_clock_alarm_init() != OPRT_OK) {
        TAL_PR_ERR("%s: alarm service init failed", __func__);
        return NULL;
    }

    TAL_PR_DEBUG("set alarm query_method to %d", query_method);

    if (query_method == AI_CLOCK_ALARM_QUERY_BY_TIME) {
        if ((alarm_query->start_time <= 0) || (alarm_query->end_time <= 0)) {
            TAL_PR_ERR("%s: miss time info when alarm schedule by time !", __func__);
            return NULL;
        }
        TAL_PR_DEBUG("set alarm query time from '%d' to %d", alarm_query->start_time, alarm_query->end_time);
    } else if (query_method == AI_CLOCK_ALARM_QUERY_ALL) {
        TAL_PR_DEBUG("set alarm query all");
    } else {
        TAL_PR_ERR("%s: unknown query method '%d' !", __func__, query_method);
        return NULL;
    }

    content = clock_alarm_query_by_query_method(query_method, alarm_query);
    if (content != NULL) {
        TAL_PR_DEBUG("%s. query content: '%s' !", __func__, content);
    }

    return content;
}

OPERATE_RET wukong_clock_schedule_list_register(TY_AI_CLOCK_LIST_S *sched_list)
{
    __SchedList = sched_list;
    return OPRT_OK;
}

OPERATE_RET wukong_clock_alarm_list_register(TY_AI_CLOCK_LIST_S *alarm_list)
{
    if (alarm_list == NULL) {
        return OPRT_INVALID_PARM;
    }

    __AlarmList = alarm_list;
    __AlarmListOwned = FALSE;
    return wukong_clock_alarm_init();
}
