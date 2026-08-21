#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "types.h"
#include "app_config.h"

void EEPROM_Init(void);
u8 EEPROM_WriteByte(u16 memAddr, u8 data);
u8 EEPROM_ReadByte(u16 memAddr, u8 *data);
u8 EEPROM_WriteBuffer(u16 memAddr, const u8 *data, u16 len);
u8 EEPROM_ReadBuffer(u16 memAddr, u8 *data, u16 len);
u8 EEPROM_WritePage(u16 memAddr, const u8 *data, u16 len);

u8 EEPROM_WriteDefaultConfig(void);
u8 EEPROM_LoadConfig(SystemConfig *cfg);
u8 EEPROM_SaveConfig(const SystemConfig *cfg);
u8 EEPROM_SaveTempSetPoint(SystemConfig *cfg, s16 temp);
u8 EEPROM_SaveHumiditySetPoint(SystemConfig *cfg, u8 humidity);
u8 EEPROM_SavePassword(SystemConfig *cfg, const char *newPassword);

#endif
