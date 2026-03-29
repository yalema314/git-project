#include "skill_clock.h"
#include "wukong_ai_agent.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_time_service.h"
#include "mix_method.h"
#include <stdio.h>

STATIC TY_AI_CLOCK_LIST_S *__SchedList = NULL;

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

OPERATE_RET wukong_clock_set_countdown_timer(TY_AI_CLOCK_TIMER_OPR_TYPE_E opr, INT_T hours, INT_T minutes, INT_T seconds)
{
    UINT_T offset = 0;
    UINT8_T *msg = NULL;
    INT_T len = 0;

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

OPERATE_RET wukong_clock_schedule_list_register(TY_AI_CLOCK_LIST_S *sched_list)
{
    __SchedList = sched_list;
    return OPRT_OK;
}
