#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "i2c.h"

#define SI_BIT      3
#define STO_BIT     4
#define STA_BIT     5
#define I2EN_BIT    6

/* Timeout for SI-wait loops.  At 60 MHz each iteration is ~4 instructions,
 * so 100 000 iterations ≈ 6-7 ms — far longer than any valid I2C
 * clock cycle.  A timeout means the bus is stuck.                        */
#define I2C_TIMEOUT 100000UL

/* Wait for SI flag with timeout.  Returns 1 on success, 0 on timeout. */
static u8 I2C0_WaitSI(void)
{
    u32 t = I2C_TIMEOUT;
    while(((I2C0CONSET >> SI_BIT) & 1U) == 0U)
    {
        if(--t == 0U) return 0U;
    }
    return 1U;
}

void I2C0_Init(void)
{
    /* Configure I/O pin for SCL & SDA functions using PINSEL0 */
    PINSEL0 &= ~0x000000F0UL;
    PINSEL0 |=  0x00000050UL;

    /* Configure Speed for I2C Serial Communication */
    I2C0SCLL = 75U;
    I2C0SCLH = 75U;

    /* I2C Peripheral Enable for Communication */
    I2C0CONSET = 1UL << I2EN_BIT;
}

u8 I2C0_Start(void)
{
    /* start condition */
    I2C0CONSET = 1UL << STA_BIT;
    /* clear SI to kick the I2C engine */
    I2C0CONCLR = 1UL << SI_BIT;
    /* wait for SI bit status */
    if(!I2C0_WaitSI()) return 0xF8U;
    /* clear start condition */
    I2C0CONCLR = 1UL << STA_BIT;
    
    return (u8)I2C0STAT;
}

u8 I2C0_Restart(void)
{
    /* start condition */
    I2C0CONSET = 1UL << STA_BIT;
    /* clr SI_BIT */
    I2C0CONCLR = 1UL << SI_BIT;
    /* wait for SI bit status */
    if(!I2C0_WaitSI()) return 0xF8U;
    /* clear start condition */
    I2C0CONCLR = 1UL << STA_BIT;
    
    return (u8)I2C0STAT;
}

void I2C0_Stop(void)
{
    /* issue stop condition */
    I2C0CONSET = 1UL << STO_BIT;
    /* clr SI bit status */
    I2C0CONCLR = 1UL << SI_BIT;
    /* stop will cleared automatically */
}

u8 I2C0_Write(u8 data)
{
    /* put data into I2DAT */
    I2C0DAT = data;
    /* clr SI_BIT */
    I2C0CONCLR = 1UL << SI_BIT;
    /* wait for SI bit status */
    if(!I2C0_WaitSI()) return 0xF8U;
    
    return (u8)I2C0STAT;
}

u8 I2C0_ReadAck(void)
{
    I2C0CONSET = 0x04U; /* Assert Ack (AA bit) */
    I2C0CONCLR = 1UL << SI_BIT;
    if(!I2C0_WaitSI()) return 0xFFU;
    I2C0CONCLR = 0x04U; /* Clear AA after read */
    
    return (u8)I2C0DAT;
}

u8 I2C0_ReadNack(void)
{
    I2C0CONCLR = 0x04U; /* Clear AA bit → master sends NACK */
    I2C0CONCLR = 1UL << SI_BIT;
    if(!I2C0_WaitSI()) return 0xFFU;
    
    return (u8)I2C0DAT;
}
