#ifndef __PASSWORD_H__
#define __PASSWORD_H__

#include "types.h"
#include "app_config.h"

u8 Password_Read(const char *prompt, char *out, u8 maxLen);
u8 Password_Verify(const SystemConfig *cfg);
u8 Password_Change(SystemConfig *cfg);

#endif
