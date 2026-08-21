#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "types.h"

/* Buzzer is ON HOLD — all functions are no-ops.
 * menu.c and password.c call these; keeping the header
 * means zero changes needed in those files.
 * When you are ready to enable the buzzer:
 *   1. Replace this header with the real buzzer.h
 *   2. Replace buzzer.c with the real buzzer.c
 *   3. Add Buzzer_Init() call back into Init_All() in main.c
 */

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Beep(u8 count, u16 onMs, u16 offMs);

#endif
