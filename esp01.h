#ifndef __ESP01_H__
#define __ESP01_H__

#include "types.h"

/* ---------------------------------------------------------------
 * Public API — called from main.c
 * --------------------------------------------------------------- */
u8   ESP01_Init(void);
u8   ESP01_SendUpdate(signed int temperature, unsigned int humidity,
                      unsigned int doorOpen,  unsigned int alarmCode);
u8   ESP01_SendField(u8 fieldNo, signed int value);
void ESP01_GetLastResponse(char *dest, u16 maxLen);

/* ---------------------------------------------------------------
 * Compatibility names used elsewhere in the project
 * --------------------------------------------------------------- */
void esp01_connectAP(void);
void esp01_sendToThingspeak(char *val);

#endif
