#ifndef __DELAY_H__
#define __DELAY_H__

#include "types.h"

/* Call once at startup before any delay function is used */
void Timer0_Init(void);

void delay_us(u32 dlyUS);
void delay_ms(u32 dlyMS);
void delay_s(u32 dlyS);

#endif
