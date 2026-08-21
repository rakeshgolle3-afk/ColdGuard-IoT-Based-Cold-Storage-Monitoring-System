#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "types.h"
#include "config.h"

typedef struct
{
    s16 tempSetPoint;
    u8  humiditySetPoint;
    u8  passwordLen;
    char password[PASSWORD_MAX_LEN + 1];
} SystemConfig;

#endif
