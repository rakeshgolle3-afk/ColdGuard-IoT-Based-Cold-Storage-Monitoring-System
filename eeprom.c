#include <string.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "i2c.h"
#include "eeprom.h"

/* AT24C256 with A2/A1/A0 tied to GND: 7-bit address 0x50.
 * Control bytes: 0xA0 write, 0xA1 read.
 */
#define EEPROM_SLA_W        0xA0U
#define EEPROM_SLA_R        0xA1U

#define EEPROM_SIG_HIGH     0xC0U
#define EEPROM_SIG_LOW      0xDEU

#define EE_ADDR_SIG_H       0x0000U
#define EE_ADDR_SIG_L       0x0001U
#define EE_ADDR_TEMP_H      0x0002U
#define EE_ADDR_TEMP_L      0x0003U
#define EE_ADDR_HUMIDITY    0x0004U
#define EE_ADDR_PASS_LEN    0x0005U
#define EE_ADDR_PASSWORD    0x0006U  /* 0x0006..0x0009 (PASSWORD_MAX_LEN=4 bytes) */
#define EE_ADDR_CHECKSUM    0x000EU
#define EE_PROJECT_BYTES    0x0010U

static u8 EEPROM_Checksum(const SystemConfig *cfg)
{
    u8 sum;
    u8 k;

    sum = 0U;
    sum = (u8)(sum + EEPROM_SIG_HIGH + EEPROM_SIG_LOW);
    sum = (u8)(sum + ((u16)cfg->tempSetPoint >> 8U));
    sum = (u8)(sum + ((u16)cfg->tempSetPoint & 0xFFU));
    sum = (u8)(sum + cfg->humiditySetPoint + cfg->passwordLen);

    for(k = 0U; k < PASSWORD_MAX_LEN; k++)
    {
        sum = (u8)(sum + (u8)cfg->password[k]);
    }
    return sum;
}

static void EEPROM_FillDefaults(SystemConfig *cfg)
{
    u8 k;

    cfg->tempSetPoint = DEFAULT_TEMP_SETPOINT;
    cfg->humiditySetPoint = DEFAULT_HUMIDITY_SETPOINT;
    cfg->passwordLen = (u8)strlen(DEFAULT_PASSWORD);
    if(cfg->passwordLen > PASSWORD_MAX_LEN)
    {
        cfg->passwordLen = PASSWORD_MAX_LEN;
    }

    for(k = 0U; k <= PASSWORD_MAX_LEN; k++)
    {
        cfg->password[k] = '\0';
    }
    strncpy(cfg->password, DEFAULT_PASSWORD, PASSWORD_MAX_LEN);
    cfg->password[PASSWORD_MAX_LEN] = '\0';
}

void EEPROM_Init(void)
{
    I2C0_Init();
}

u8 EEPROM_WriteByte(u16 memAddr, u8 data)
{
    u8 status;

    status = I2C0_Start();
    if((status != 0x08U) && (status != 0x10U))
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write(EEPROM_SLA_W);
    if(status != 0x18U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write((u8)(memAddr >> 8U));
    if(status != 0x28U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write((u8)(memAddr & 0xFFU));
    if(status != 0x28U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write(data);
    if(status != 0x28U)
    {
        I2C0_Stop();
        return 0U;
    }

    I2C0_Stop();
    delay_ms(10U); /* EEPROM internal write cycle */
    return 1U;
}

u8 EEPROM_ReadByte(u16 memAddr, u8 *data)
{
    u8 status;

    status = I2C0_Start();
    if((status != 0x08U) && (status != 0x10U))
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write(EEPROM_SLA_W);
    if(status != 0x18U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write((u8)(memAddr >> 8U));
    if(status != 0x28U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write((u8)(memAddr & 0xFFU));
    if(status != 0x28U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Restart(); /* repeated start */
    if((status != 0x10U) && (status != 0x08U))
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write(EEPROM_SLA_R);
    if(status != 0x40U)
    {
        I2C0_Stop();
        return 0U;
    }

    *data = I2C0_ReadNack();
    I2C0_Stop();
    return 1U;
}

u8 EEPROM_WriteBuffer(u16 memAddr, const u8 *data, u16 len)
{
    u16 k;

    for(k = 0U; k < len; k++)
    {
        if(!EEPROM_WriteByte((u16)(memAddr + k), data[k]))
        {
            return 0U;
        }
    }
    return 1U;
}

u8 EEPROM_ReadBuffer(u16 memAddr, u8 *data, u16 len)
{
    u16 k;

    for(k = 0U; k < len; k++)
    {
        if(!EEPROM_ReadByte((u16)(memAddr + k), &data[k]))
        {
            return 0U;
        }
    }
    return 1U;
}

u8 EEPROM_WritePage(u16 memAddr, const u8 *data, u16 len)
{
    u8 status;
    u16 k;

    status = I2C0_Start();
    if((status != 0x08U) && (status != 0x10U))
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write(EEPROM_SLA_W);
    if(status != 0x18U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write((u8)(memAddr >> 8U));
    if(status != 0x28U)
    {
        I2C0_Stop();
        return 0U;
    }

    status = I2C0_Write((u8)(memAddr & 0xFFU));
    if(status != 0x28U)
    {
        I2C0_Stop();
        return 0U;
    }

    for(k = 0U; k < len; k++)
    {
        status = I2C0_Write(data[k]);
        if(status != 0x28U)
        {
            I2C0_Stop();
            return 0U;
        }
    }

    I2C0_Stop();
    delay_ms(10U); /* wait for write cycle */
    return 1U;
}

u8 EEPROM_SaveConfig(const SystemConfig *cfg)
{
    u8 buf[EE_ADDR_CHECKSUM + 1U];
    u8 checksum;
    u16 tempVal;
    u8 k;

    if((cfg->passwordLen < PASSWORD_MIN_LEN) || (cfg->passwordLen > PASSWORD_MAX_LEN))
    {
        return 0U;
    }

    /* Zero entire buffer first so any gap bytes between the password
     * end and EE_ADDR_CHECKSUM are written as 0x00, not stack garbage. */
    for(k = 0U; k < (u8)sizeof(buf); k++) { buf[k] = 0x00U; }

    tempVal  = (u16)cfg->tempSetPoint;
    checksum = EEPROM_Checksum(cfg);

    buf[EE_ADDR_SIG_H]    = EEPROM_SIG_HIGH;
    buf[EE_ADDR_SIG_L]    = EEPROM_SIG_LOW;
    buf[EE_ADDR_TEMP_H]   = (u8)(tempVal >> 8U);
    buf[EE_ADDR_TEMP_L]   = (u8)(tempVal & 0xFFU);
    buf[EE_ADDR_HUMIDITY] = cfg->humiditySetPoint;
    buf[EE_ADDR_PASS_LEN] = cfg->passwordLen;

    for(k = 0U; k < PASSWORD_MAX_LEN; k++)
    {
        buf[EE_ADDR_PASSWORD + k] = (u8)cfg->password[k];
    }
    buf[EE_ADDR_CHECKSUM] = checksum;

    return EEPROM_WritePage(0x0000U, buf, (u16)sizeof(buf));
}

u8 EEPROM_WriteDefaultConfig(void)
{
    SystemConfig cfg;

    EEPROM_FillDefaults(&cfg);

    /* SaveConfig() already zeroes its buffer before packing, so a
     * separate byte-by-byte erase is unnecessary.               */
    return EEPROM_SaveConfig(&cfg);
}

static u8 EEPROM_ReadConfigRaw(SystemConfig *cfg)
{
    u8 sigH;
    u8 sigL;
    u8 tempH;
    u8 tempL;
    u8 checksumStored;
    u8 checksumCalc;
    u8 k;

    if(!EEPROM_ReadByte(EE_ADDR_SIG_H, &sigH)) return 0U;
    if(!EEPROM_ReadByte(EE_ADDR_SIG_L, &sigL)) return 0U;
    if((sigH != EEPROM_SIG_HIGH) || (sigL != EEPROM_SIG_LOW)) return 0U;

    if(!EEPROM_ReadByte(EE_ADDR_TEMP_H, &tempH)) return 0U;
    if(!EEPROM_ReadByte(EE_ADDR_TEMP_L, &tempL)) return 0U;
    cfg->tempSetPoint = (s16)(((u16)tempH << 8U) | tempL);

    if(!EEPROM_ReadByte(EE_ADDR_HUMIDITY, &cfg->humiditySetPoint)) return 0U;
    if(!EEPROM_ReadByte(EE_ADDR_PASS_LEN, &cfg->passwordLen)) return 0U;

    if((cfg->passwordLen < PASSWORD_MIN_LEN) || (cfg->passwordLen > PASSWORD_MAX_LEN))
    {
        return 0U;
    }

    for(k = 0U; k < PASSWORD_MAX_LEN; k++)
    {
        if(!EEPROM_ReadByte((u16)(EE_ADDR_PASSWORD + k), (u8 *)&cfg->password[k])) return 0U;
    }
    cfg->password[PASSWORD_MAX_LEN] = '\0';

    if(!EEPROM_ReadByte(EE_ADDR_CHECKSUM, &checksumStored)) return 0U;
    checksumCalc = EEPROM_Checksum(cfg);
    if(checksumCalc != checksumStored) return 0U;

    return 1U;
}

u8 EEPROM_LoadConfig(SystemConfig *cfg)
{
#if EEPROM_FIRST_TIME_SETUP
    if(!EEPROM_WriteDefaultConfig())
    {
        EEPROM_FillDefaults(cfg);
        return 0U;
    }
#endif

    if(EEPROM_ReadConfigRaw(cfg))
    {
        return 1U;
    }

    /* If EEPROM is blank/corrupted, recover with safe defaults. */
    EEPROM_FillDefaults(cfg);
    EEPROM_SaveConfig(cfg);
    return 0U;
}

u8 EEPROM_SaveTempSetPoint(SystemConfig *cfg, s16 temp)
{
    cfg->tempSetPoint = temp;
    return EEPROM_SaveConfig(cfg);
}

u8 EEPROM_SaveHumiditySetPoint(SystemConfig *cfg, u8 humidity)
{
    cfg->humiditySetPoint = humidity;
    return EEPROM_SaveConfig(cfg);
}

u8 EEPROM_SavePassword(SystemConfig *cfg, const char *newPassword)
{
    u8 len;
    u8 k;

    len = (u8)strlen(newPassword);
    if((len < PASSWORD_MIN_LEN) || (len > PASSWORD_MAX_LEN))
    {
        return 0U;
    }

    for(k = 0U; k <= PASSWORD_MAX_LEN; k++)
    {
        cfg->password[k] = '\0';
    }
    strncpy(cfg->password, newPassword, PASSWORD_MAX_LEN);
    cfg->password[PASSWORD_MAX_LEN] = '\0';
    cfg->passwordLen = len;

    return EEPROM_SaveConfig(cfg);
}
