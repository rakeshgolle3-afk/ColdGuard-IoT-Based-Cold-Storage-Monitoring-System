#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "dht11.h"

#define DHT11_MASK      (1UL << DHT11_DATA_PIN)

/* ---------------------------------------------------------------
 * Timeout for polling loops.
 * At CCLK=60MHz each loop iteration is ~3-4 instructions.
 * 100000 iterations ≈ 5-7 ms — far longer than any valid DHT11
 * pulse, so a timeout means the sensor is genuinely not responding.
 * --------------------------------------------------------------- */
#define DHT11_TIMEOUT   100000U

/* ---------------------------------------------------------------
 * Pin helpers
 * --------------------------------------------------------------- */
static void DHT11_SetOutput(void)
{
    IODIR0 |= DHT11_MASK;
}

static void DHT11_SetInput(void)
{
    IODIR0 &= ~DHT11_MASK;   /* tri-state: pull-up resistor takes pin HIGH */
}

static void DHT11_High(void)
{
    IOSET0 = DHT11_MASK;
}

static void DHT11_Low(void)
{
    IOCLR0 = DHT11_MASK;
}

static u8 DHT11_PinRead(void)
{
    return (u8)((IOPIN0 & DHT11_MASK) ? 1U : 0U);
}

/* Wait until pin == state.  Returns 1 on success, 0 on timeout. */
static u8 DHT11_Wait(u8 state, u32 timeout)
{
    while(DHT11_PinRead() != state)
    {
        if(timeout == 0U) { return 0U; }
        timeout--;
    }
    return 1U;
}

/* ---------------------------------------------------------------
 * Read one byte (8 bits) from the bus.
 * Must be called with IRQs already disabled.
 * Returns 0xFF if any bit times out (caller checks checksum anyway).
 * --------------------------------------------------------------- */
static u8 DHT11_ReadByte(void)
{
    u8 i;
    u8 result = 0U;

    for(i = 0U; i < 8U; i++)
    {
        /* Each bit starts with a 50 µs LOW pulse from DHT11 */
        if(!DHT11_Wait(0U, DHT11_TIMEOUT)) { return 0xFFU; }   /* wait LOW start  */
        if(!DHT11_Wait(1U, DHT11_TIMEOUT)) { return 0xFFU; }   /* wait HIGH start */

        /* Sample at 40 µs into the HIGH period:
         *   "0" bit  = HIGH for ~26-28 µs  → pin is LOW  at 40 µs
         *   "1" bit  = HIGH for ~70 µs     → pin is HIGH at 40 µs */
        delay_us(40U);

        result = (u8)(result << 1U);
        if(DHT11_PinRead())
        {
            result |= 0x01U;
            /* Wait for HIGH to end so next bit's LOW edge is clean */
            if(!DHT11_Wait(0U, DHT11_TIMEOUT)) { return 0xFFU; }
        }
        /* If pin is already 0 at 40 µs we're ready for next bit */
    }
    return result;
}

/* ---------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------- */
void DHT11_Init(void)
{
    /* Configure P0.DHT11_DATA_PIN as GPIO, output, idle HIGH */
    PINSEL0 &= ~(3UL << (DHT11_DATA_PIN * 2U));
    DHT11_SetOutput();
    DHT11_High();

    /* DHT11 datasheet: wait ≥ 1 s after power-on before first read */
    delay_ms(1200U);
}

u8 DHT11_Read(DHT11_Data *data)
{
    u8 calcChecksum;

    /* --- START SIGNAL ------------------------------------------------
     * Host pulls bus LOW for ≥ 18 ms, then releases (HIGH).
     * Do this with IRQs enabled — 20 ms is a long blocking delay but
     * it doesn't need microsecond precision.
     * ----------------------------------------------------------------- */
    DHT11_SetOutput();
    DHT11_Low();
    delay_ms(20U);          /* >18 ms start pulse                       */
    DHT11_High();
    delay_us(30U);          /* Hold HIGH 20-40 µs before releasing bus  */
    DHT11_SetInput();       /* Release: pull-up takes bus HIGH          */

    /* --- DISABLE INTERRUPTS -----------------------------------------
     * Everything from here to the end of data reception must run
     * without interruption.  A single missed microsecond can corrupt
     * the bit stream.
     * ----------------------------------------------------------------- */
    __disable_irq();   /* Keil built-in: disables IRQ and FIQ */

    /* --- DHT11 RESPONSE HANDSHAKE ------------------------------------
     * DHT11 responds: pull LOW 80 µs, then HIGH 80 µs.
     * ----------------------------------------------------------------- */
    if(!DHT11_Wait(0U, DHT11_TIMEOUT)) { goto fail; }  /* wait for 80 µs LOW  */
    if(!DHT11_Wait(1U, DHT11_TIMEOUT)) { goto fail; }  /* wait for 80 µs HIGH */
    if(!DHT11_Wait(0U, DHT11_TIMEOUT)) { goto fail; }  /* wait HIGH to end    */

    /* --- READ 5 BYTES ------------------------------------------------ */
    data->humidity_integer = DHT11_ReadByte();
    data->humidity_decimal = DHT11_ReadByte();
    data->temp_integer     = DHT11_ReadByte();
    data->temp_decimal     = DHT11_ReadByte();
    data->checksum         = DHT11_ReadByte();

    /* --- RE-ENABLE INTERRUPTS ---------------------------------------- */
    __enable_irq();    /* Keil built-in: re-enables IRQ */

    /* --- Bus returns IDLE HIGH --------------------------------------- */
    DHT11_SetOutput();
    DHT11_High();

    /* --- CHECKSUM ---------------------------------------------------- */
    calcChecksum = (u8)(  data->humidity_integer
                        + data->humidity_decimal
                        + data->temp_integer
                        + data->temp_decimal);

    if(calcChecksum != data->checksum) { return 0U; }

    return 1U;

fail:
    /* Re-enable interrupts and leave bus idle */
    __enable_irq();    /* Keil built-in: re-enables IRQ */

    DHT11_SetOutput();
    DHT11_High();
    return 0U;
}
