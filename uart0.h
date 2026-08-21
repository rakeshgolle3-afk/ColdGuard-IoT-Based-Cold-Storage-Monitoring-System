#ifndef __UART0_H__
#define __UART0_H__

#include "types.h"

/* ---------------------------------------------------------------
 * Shared receive buffer — filled by UART0_ISR.
 * esp01.c declares these extern to read/reset the buffer.
 * --------------------------------------------------------------- */
extern char          buff[];        /* receive buffer (size = ESP_RESPONSE_BUFFER_SIZE) */
extern volatile unsigned int  i;             /* write index into buff[] */
extern unsigned char r_flag;        /* set by ISR when new data arrives */

/* ---------------------------------------------------------------
 * Core UART0 API (uppercase style)
 * --------------------------------------------------------------- */
void UART0_Init(void);
void UART0_Tx(char ch);
char UART0_Rx(void);
u8   UART0_RxReady(void);
void UART0_ClearRx(void);
void UART0_Str(const char *s);
void UART0_Int(signed int n);
void UART0_UInt(unsigned int n);
void UART0_Float(float f, char digitsAfterDecimal);

/* ---------------------------------------------------------------
 * Lowercase aliases — match the working reference code style
 * used directly inside esp01.c
 * --------------------------------------------------------------- */
void uart0_str(u8 *s);
void uart0_int(u32 n);
void uart0_float(f32 f);

/* ---------------------------------------------------------------
 * Compatibility wrappers — keep existing call-sites building
 * --------------------------------------------------------------- */
void          InitUART0(void);
void          Init_UART0(void);
void          UART0_Tx_char(unsigned char dat);
unsigned char UART0_Rx_char(void);
void          UART0_Tx_str(char *p);
void          UART0_Tx_int(signed int n);
void          UART0_Tx_float(float f, char digitsAfterDecimal);

#endif
