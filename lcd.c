#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "lcd.h"

#define LCD_DATA_MASK  (0xFFUL << LCD_DATA_START_PIN)
#define LCD_RS_MASK    (1UL << LCD_RS_PIN)
#define LCD_EN_MASK    (1UL << LCD_EN_PIN)

/* ---------------------------------------------------------------
 * Software delay loops for LCD timing.
 * These do NOT depend on Timer0 — they work in both Proteus
 * simulation and real hardware.  The loop counts are calibrated
 * for CCLK = 60 MHz (each iteration ≈ 3-4 instructions).
 * Exact timing is not critical for the LCD — we just need to
 * exceed the HD44780 minimum pulse widths.
 * --------------------------------------------------------------- */
static void LCD_delay_us(u32 us)
{
    volatile u32 count;
    /* ~30 iterations per microsecond at 60 MHz (rough estimate;
     * deliberately over-counts so delays are always long enough for Proteus) */
    for(count = 0U; count < (us * 30U); count++)
    {
        ; /* empty body — volatile prevents optimisation */
    }
}

static void LCD_delay_ms(u32 ms)
{
    while(ms--)
    {
        LCD_delay_us(1000U);
    }
}

static void LCD_PulseEnable(void)
{
    IOSET0 = LCD_EN_MASK;
    LCD_delay_us(5U);   /* HD44780 needs EN high >= 450 ns; 5 us margin */
    IOCLR0 = LCD_EN_MASK;
    LCD_delay_us(5U);   /* EN low hold time before next operation */
}

void LCD_Init(void)
{
    /* --- PINSEL configuration ---
     * LPC2148 PINSEL uses 2 bits per pin; writing 00 = GPIO mode.
     *
     * PINSEL0 [31:0]  -> P0.0..P0.15  (2 bits each)
     *   bits [31:16] cover P0.8..P0.15  -> mask 0xFFFF0000
     *
     * PINSEL1 [31:0]  -> P0.16..P0.31 (2 bits each)
     *   bits [15:0]  cover P0.16..P0.23 -> we need bits [15:0] cleared
     *   for P0.12..P0.15 (D4..D7) we need PINSEL1 bits [15:0] as well:
     *   Wait — P0.12..P0.15 are in PINSEL0 bits [31:24], already covered above.
     *   P0.16 (RS) = PINSEL1 bits [1:0]
     *   P0.17 (EN) = PINSEL1 bits [3:2]
     *   So clear PINSEL1 bits [3:0] for RS and EN. */

    /* P0.8..P0.15 as GPIO (data bus D0..D7) */
    PINSEL0 &= ~0xFFFF0000UL;   /* bits[31:16] -> P0.8..P0.15 */

    /* P0.16 (RS) and P0.17 (EN) as GPIO */
    PINSEL1 &= ~0x0000000FUL;   /* bits[3:0]   -> P0.16, P0.17 */

    /* Set all LCD pins as output and drive low */
    IODIR0 |= LCD_DATA_MASK | LCD_RS_MASK | LCD_EN_MASK;
    IOCLR0  = LCD_DATA_MASK | LCD_RS_MASK | LCD_EN_MASK;

    /* HD44780 power-on reset sequence (datasheet section 4, p.45)
     * Wait > 40 ms after VCC rises to 2.7 V */
    LCD_delay_ms(50U);

    /* Function-set sent three times to guarantee 8-bit mode entry
     * even if the controller was in an unknown state */
    Write_CMD_LCD(0x30);
    LCD_delay_ms(5U);   /* must wait > 4.1 ms after first function set */
    Write_CMD_LCD(0x30);
    LCD_delay_ms(1U);   /* must wait > 100 us */
    Write_CMD_LCD(0x30);
    LCD_delay_ms(1U);

    Write_CMD_LCD(0x38);    /* 8-bit, 2-line, 5x7 font */
    LCD_delay_ms(1U);
    Write_CMD_LCD(0x08);    /* Display OFF (before clear, per datasheet) */
    LCD_delay_ms(1U);
    Write_CMD_LCD(0x01);    /* Clear display */
    LCD_delay_ms(2U);       /* Clear needs > 1.64 ms */
    Write_CMD_LCD(0x06);    /* Entry mode: increment, no shift */
    LCD_delay_ms(1U);
    Write_CMD_LCD(0x0C);    /* Display ON, cursor OFF, blink OFF */
    LCD_delay_ms(1U);
}

void Write_LCD(char ch)
{
    IOCLR0 = LCD_DATA_MASK;
    IOSET0 = (((u32)ch << LCD_DATA_START_PIN) & LCD_DATA_MASK);
    LCD_PulseEnable();
    LCD_delay_ms(2U);
}

void Write_CMD_LCD(char cmd)
{
    IOCLR0 = LCD_RS_MASK;
    Write_LCD(cmd);
    if((cmd == 0x01) || (cmd == 0x02))
    {
        LCD_delay_ms(2U);
    }
}

void Write_DAT_LCD(char dat)
{
    IOSET0 = LCD_RS_MASK;
    Write_LCD(dat);
}

void Write_str_LCD(const char *p)
{
    while(*p)
    {
        Write_DAT_LCD(*p++);
    }
}

void Write_int_LCD(signed int n)
{
    char a[11]; /* max 10 digits + sign */
    char i;
    unsigned int un;

    i = 0;
    /* Use unsigned arithmetic to avoid undefined behaviour when n == INT_MIN
     * (negating INT_MIN with signed int overflows). */
    if(n < 0)
    {
        Write_DAT_LCD('-');
        un = (unsigned int)(-(n + 1)) + 1U; /* safe two-step negate */
    }
    else
    {
        un = (unsigned int)n;
    }

    if(un == 0U)
    {
        Write_DAT_LCD('0');
        return;
    }

    do
    {
        a[(int)i++] = (char)((un % 10U) + '0');
        un = un / 10U;
    } while(un != 0U);

    while(i > 0)
    {
        Write_DAT_LCD(a[(int)--i]);
    }
}

void Write_float_LCD(float f, char digitsAfterDecimal)
{
    unsigned long int n;

    n = (unsigned long int)f;
    Write_int_LCD((signed int)n);
    Write_DAT_LCD('.');

    while(digitsAfterDecimal-- > 0)
    {
        f = f * 10.0f;
        n = (unsigned long int)f;
        Write_DAT_LCD((char)((n % 10U) + '0'));
    }
}

void LCD_Clear(void)
{
    Write_CMD_LCD(0x01);
    LCD_delay_ms(2U);
}

void LCD_Goto(u8 row, u8 col)
{
    u8 addr;

    if(row == 0U)
    {
        addr = (u8)(0x80U + col);
    }
    else
    {
        addr = (u8)(0xC0U + col);
    }
    Write_CMD_LCD((char)addr);
}

void LCD_PrintPadded(const char *text, u8 width)
{
    u8 count;

    count = 0U;
    while((*text) && (count < width))
    {
        Write_DAT_LCD(*text++);
        count++;
    }
    while(count < width)
    {
        Write_DAT_LCD(' ');
        count++;
    }
}

void LCD_Print2Lines(const char *line1, const char *line2)
{
    LCD_Clear();
    LCD_Goto(0U, 0U);
    LCD_PrintPadded(line1, 16U);
    LCD_Goto(1U, 0U);
    LCD_PrintPadded(line2, 16U);
}
