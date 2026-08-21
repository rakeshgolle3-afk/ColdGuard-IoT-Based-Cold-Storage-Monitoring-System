#ifndef __DOOR_INTERRUPT_H__
#define __DOOR_INTERRUPT_H__

#include "types.h"

void DoorInterrupt_Init(void);
u8 Door_IsOpen(void);
u8 MenuSwitch_IsPressed(void);
u8 Door_GetAndClearOpenEvent(void);
u8 Menu_GetAndClearEvent(void);

#endif
