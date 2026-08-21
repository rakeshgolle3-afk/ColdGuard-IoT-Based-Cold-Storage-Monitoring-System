#ifndef __DHT11_H__
#define __DHT11_H__

#include "types.h"

typedef struct
{
    u8 humidity_integer;
    u8 humidity_decimal;
    u8 temp_integer;
    u8 temp_decimal;
    u8 checksum;
} DHT11_Data;

void DHT11_Init(void);

/* Returns 1 on success, 0 on timeout or checksum failure.
 * Disables all IRQs during the read to protect microsecond timing. */
u8   DHT11_Read(DHT11_Data *data);

#endif
