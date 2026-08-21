#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "delay.h"

/* ---------------------------------------------------------------
 * Normal software delays.
 * --------------------------------------------------------------- */

void Timer0_Init(void)
{
    /* PCLK = CCLK/4 = 15 MHz */
    T0TCR = 0x02; /* Reset Timer0 */
    T0PR  = 14;   /* Prescaler: 15MHz / 15 = 1 MHz -> 1 us per tick */
    T0TCR = 0x01; /* Enable Timer0 */
}

/* Block for 'dlyUS' microseconds using Timer0 */
void delay_us(u32 dlyUS)
{
    u32 start = T0TC;
    while ((T0TC - start) < dlyUS)
    {
        /* Wait */
    }
}

/* Block for 'dlyMS' milliseconds using Timer0 */
void delay_ms(u32 dlyMS)
{
    u32 start = T0TC;
    u32 ticks = dlyMS * 1000U;
    while ((T0TC - start) < ticks)
    {
        /* Wait */
    }
}

/* Block for 'dlyS' seconds */
void delay_s(u32 dlyS)
{
    while(dlyS--)
    {
        delay_ms(1000U);
    }
}
