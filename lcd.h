#ifndef __LCD_H__
#define __LCD_H__

#include "types.h"

void LCD_Init(void);
void LCD_Clear(void);
void LCD_Goto(u8 row, u8 col);
void LCD_Print2Lines(const char *line1, const char *line2);
void LCD_PrintPadded(const char *text, u8 width);

/* Names retained from your supplied code */
void Write_CMD_LCD(char cmd);
void Write_DAT_LCD(char dat);
void Write_LCD(char ch);
void Write_str_LCD(const char *p);
void Write_int_LCD(signed int n);
void Write_float_LCD(float f, char digitsAfterDecimal);

#endif
