#ifndef __I2C_H__
#define __I2C_H__

#include "types.h"

void I2C0_Init(void);
u8 I2C0_Start(void);
u8 I2C0_Restart(void);
void I2C0_Stop(void);
u8 I2C0_Write(u8 data);
u8 I2C0_ReadAck(void);
u8 I2C0_ReadNack(void);

#endif
