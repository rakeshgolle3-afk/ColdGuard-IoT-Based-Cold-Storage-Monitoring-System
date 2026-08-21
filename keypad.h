#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include "types.h"

void Keypad_Init(void);
char Keypad_GetKey(void);
char Keypad_GetKeyNonBlocking(void);

#endif
