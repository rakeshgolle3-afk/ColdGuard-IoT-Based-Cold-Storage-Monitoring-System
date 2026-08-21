#ifndef __RTC_H__
#define __RTC_H__

#include "types.h"

typedef struct {
    u16 year;
    u8 month;
    u8 dom;
    u8 hour;
    u8 min;
    u8 sec;
} RTC_Time;

void RTC_Init(void);
void RTC_SetTime(const RTC_Time *time);
void RTC_GetTime(RTC_Time *time);

#endif /* __RTC_H__ */
