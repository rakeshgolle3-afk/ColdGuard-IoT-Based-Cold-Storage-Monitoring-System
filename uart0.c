#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "uart0.h"

/* ---------------------------------------------------------------
 * Global shared receive buffer — filled by ISR.
 * esp01.c uses extern to access buff[] and i.
 * Size matches ESP response buffer size in config.h.
 * --------------------------------------------------------------- */
char          buff[ESP_RESPONSE_BUFFER_SIZE];
volatile unsigned int  i = 0U;
unsigned char r_flag   = 0U;

/* ---------------------------------------------------------------
 * UART0 Interrupt Service Routine
 * Mirrors the working reference: fills buff[] directly.
 * --------------------------------------------------------------- */
void UART0_ISR(void) __irq
{
    unsigned char iir;
    iir = (unsigned char)U0IIR;

    if((iir & 0x0EU) == 0x04U || (iir & 0x0EU) == 0x0CU)
    {
        /* Receive interrupt — drain FIFO */
        while(U0LSR & 0x01U)
        {
            unsigned char ch = (unsigned char)U0RBR;
            if(i < (unsigned int)(ESP_RESPONSE_BUFFER_SIZE - 1U))
            {
                buff[i++] = (char)ch;
                buff[i]   = '\0';
            }
        }
        r_flag = 1U;
    }
    else
    {
        (void)U0IIR; /* read to clear transmit interrupt */
    }
    VICVectAddr = 0x00UL;
}

/* ---------------------------------------------------------------
 * UART0 Initialisation
 * --------------------------------------------------------------- */
void UART0_Init(void)
{
    /* P0.0 = TXD0 (function 01), P0.1 = RXD0 (function 01) */
    PINSEL0 &= ~0x0000000FUL;
    PINSEL0 |=  0x00000005UL;

    U0LCR = 0x83U; /* 8 data bits, 1 stop bit, no parity, DLAB=1 */
    U0DLL = (u8)(UART0_DIVISOR & 0xFFU);
    U0DLM = (u8)((UART0_DIVISOR >> 8U) & 0xFFU);
    U0LCR = 0x03U; /* DLAB=0 */

    /* Reset receive buffer */
    i      = 0U;
    r_flag = 0U;
    buff[0] = '\0';

    /* Enable RDA + THRE interrupts (matches working reference) */
    U0IER = 0x03U;

    VICIntSelect &= ~(1UL << 6U);                /* UART0 = IRQ (not FIQ), preserve other bits */
    VICVectAddr3  = (unsigned long)UART0_ISR;
    VICVectCntl3  = 0x20U | 6U;                 /* slot 3, UART0 source = 6 */
    VICIntEnable  = (1UL << 6U);
}

/* ---------------------------------------------------------------
 * Transmit one character (polling)
 * --------------------------------------------------------------- */
void UART0_Tx(char ch)
{
    while(((U0LSR >> 5U) & 1U) == 0U);
    U0THR = (unsigned char)ch;
}

/* ---------------------------------------------------------------
 * Receive one character (blocking poll of U0RBR).
 * WARNING: Do NOT call while UART0 ISR is active — the ISR also
 * reads U0RBR, so data will be lost.  This function exists only
 * for compatibility; esp01.c reads from buff[] instead.
 * --------------------------------------------------------------- */
char UART0_Rx(void)
{
    while(!(U0LSR & 0x01U));
    return (char)U0RBR;
}

/* ---------------------------------------------------------------
 * Check if data is in buff (ISR-filled buffer)
 * --------------------------------------------------------------- */
u8 UART0_RxReady(void)
{
    return (u8)(i > 0U ? 1U : 0U);
}

/* ---------------------------------------------------------------
 * Clear the shared receive buffer
 * --------------------------------------------------------------- */
void UART0_ClearRx(void)
{
    i      = 0U;
    r_flag = 0U;
    buff[0] = '\0';
}

/* ---------------------------------------------------------------
 * Transmit null-terminated string
 * --------------------------------------------------------------- */
void UART0_Str(const char *s)
{
    while(*s)
        UART0_Tx(*s++);
}

/* ---------------------------------------------------------------
 * uart0_str — lowercase alias used by esp01.c (working reference style)
 * --------------------------------------------------------------- */
void uart0_str(u8 *s)
{
    while(*s)
        UART0_Tx((char)*s++);
}

/* ---------------------------------------------------------------
 * Transmit unsigned integer
 * --------------------------------------------------------------- */
void UART0_UInt(unsigned int n)
{
    char a[10];
    char idx = 0;

    if(n == 0U)
    {
        UART0_Tx('0');
        return;
    }
    while(n > 0U)
    {
        a[(int)idx++] = (char)((n % 10U) + '0');
        n = n / 10U;
    }
    while(idx > 0)
        UART0_Tx(a[(int)--idx]);
}

/* ---------------------------------------------------------------
 * Transmit signed integer
 * --------------------------------------------------------------- */
void UART0_Int(signed int n)
{
    unsigned int un;
    if(n < 0)
    {
        UART0_Tx('-');
        un = (unsigned int)(-(n + 1)) + 1U;
    }
    else
    {
        un = (unsigned int)n;
    }
    UART0_UInt(un);
}

/* ---------------------------------------------------------------
 * uart0_int — unsigned 32-bit alias (working reference style)
 * --------------------------------------------------------------- */
void uart0_int(u32 n)
{
    unsigned char a[10];
    int k = 0;
    if(n == 0U)
    {
        UART0_Tx('0');
        return;
    }
    while(n > 0U)
    {
        a[k++] = (unsigned char)((n % 10U) + '0');
        n = n / 10U;
    }
    for(--k; k >= 0; k--)
        UART0_Tx((char)a[k]);
}

/* ---------------------------------------------------------------
 * Transmit float (configurable decimal places)
 * --------------------------------------------------------------- */
void UART0_Float(float f, char digitsAfterDecimal)
{
    unsigned long int n;
    n = (unsigned long int)f;
    UART0_Int((signed int)n);
    UART0_Tx('.');
    while(digitsAfterDecimal-- > 0)
    {
        f = f * 10.0f;
        n = (unsigned long int)f;
        UART0_Tx((char)((n % 10U) + '0'));
    }
}

/* ---------------------------------------------------------------
 * uart0_float — 2-decimal alias (working reference style)
 * --------------------------------------------------------------- */
void uart0_float(f32 f)
{
    int x;
    float temp;
    x = (int)f;
    uart0_int((u32)x);
    UART0_Tx('.');
    temp = (f - (float)x) * 100.0f;
    x = (int)temp;
    uart0_int((u32)x);
}

/* ---------------------------------------------------------------
 * Compatibility wrappers (keep old call-sites working)
 * --------------------------------------------------------------- */
void InitUART0(void)                               { UART0_Init(); }
void Init_UART0(void)                              { UART0_Init(); }
void UART0_Tx_char(unsigned char dat)              { UART0_Tx((char)dat); }
unsigned char UART0_Rx_char(void)                  { return (unsigned char)UART0_Rx(); }
void UART0_Tx_str(char *p)                         { UART0_Str((const char *)p); }
void UART0_Tx_int(signed int n)                    { UART0_Int(n); }
void UART0_Tx_float(float f, char d)               { UART0_Float(f, d); }
