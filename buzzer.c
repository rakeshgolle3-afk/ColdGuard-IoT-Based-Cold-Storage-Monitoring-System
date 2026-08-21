#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "buzzer.h"

void Buzzer_Init(void)
{
    /* Set BUZZER_PIN (P0.19) as GPIO by clearing PINSEL1 bits 6:7 */
    PINSEL1 &= ~(3UL << ((BUZZER_PIN - 16) * 2));
    
    /* Configure as output */
    IODIR0 |= (1UL << BUZZER_PIN);
    
    Buzzer_Off();
}

void Buzzer_On(void)
{
#if BUZZER_ACTIVE_HIGH
    IOSET0 = (1UL << BUZZER_PIN);
#else
    IOCLR0 = (1UL << BUZZER_PIN);
#endif
}

void Buzzer_Off(void)
{
#if BUZZER_ACTIVE_HIGH
    IOCLR0 = (1UL << BUZZER_PIN);
#else
    IOSET0 = (1UL << BUZZER_PIN);
#endif
}

void Buzzer_Beep(u8 count, u16 onMs, u16 offMs)
{
    u8 i;
    for(i = 0U; i < count; i++)
    {
        Buzzer_On();
        delay_ms(onMs);
        Buzzer_Off();
        
        if (i < (count - 1U)) 
        {
            delay_ms(offMs);
        }
    }
}
