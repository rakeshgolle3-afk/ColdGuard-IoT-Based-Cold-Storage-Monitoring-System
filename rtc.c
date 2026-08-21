#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "rtc.h"

void RTC_Init(void)
{
    /* Use the 32.768 kHz external RTC crystal (board-confirmed populated).
     * CCR[0] CLKEN : RTC counter enable.
     * CCR[1] CTCRST: CTC reset (clear counter before enable).
     * CCR[4] CLKSRC: 1 = use 32 kHz oscillator input instead of PCLK.
     * When CLKSRC=1, PREINT/PREFRAC are ignored; the oscillator divides
     * internally to produce exactly 1 Hz. */
    CCR = 0x00U;            /* Stop RTC and clear all control bits */
    CCR = 0x02U;            /* CTCRST = 1: reset the CTC counter */
    CCR = (1U << 4) | 0x01U; /* CLKSRC=1 (crystal), CLKEN=1 (start) */
}

void RTC_SetTime(const RTC_Time *time)
{
    /* Disable RTC to update */
    CCR = 0x00;

    SEC = time->sec;
    MIN = time->min;
    HOUR = time->hour;
    DOM = time->dom;
    MONTH = time->month;
    YEAR = time->year;

    /* Re-enable and select 32.768 kHz external crystal */
    CCR = (1U << 4) | 0x01U;
}

void RTC_GetTime(RTC_Time *time)
{
    u32 ctime0, ctime1;

    /* Read CTIME0: SEC (0:5), MIN (8:13), HOUR (16:20), DOW (24:26) */
    ctime0 = CTIME0;
    
    /* Read CTIME1: DOM (0:4), MONTH (8:11), YEAR (16:27) */
    ctime1 = CTIME1;

    time->sec = (u8)(ctime0 & 0x3F);
    time->min = (u8)((ctime0 >> 8) & 0x3F);
    time->hour = (u8)((ctime0 >> 16) & 0x1F);
    time->dom = (u8)(ctime1 & 0x1F);
    time->month = (u8)((ctime1 >> 8) & 0x0F);
    time->year = (u16)((ctime1 >> 16) & 0xFFF);
}
