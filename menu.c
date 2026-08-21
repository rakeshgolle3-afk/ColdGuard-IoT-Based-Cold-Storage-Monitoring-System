#include "types.h"
#include "config.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"
#include "buzzer.h"
#include "password.h"
#include "eeprom.h"
#include "menu.h"

/* ---------------------------------------------------------------
 * Helper: read a decimal number from the keypad.
 *
 * Keys:
 *   0-9  : digit input (shown on LCD line 2)
 *   D    : backspace
 *   #    : confirm
 *   *    : cancel (returns 0)
 *
 * Returns 1 and writes *out if value is in [minVal, maxVal].
 * Returns 0 if user pressed *.
 * --------------------------------------------------------------- */
static u8 read_number(const char *prompt,
                       signed int  minVal,
                       signed int  maxVal,
                       signed int *out)
{
    char       key;
    char       buf[5];       /* up to 4 digits + NUL */
    u8         len;
    signed int value;
    u8         k;

    len = 0U;
    value = 0;
    for(k = 0U; k < (u8)sizeof(buf); k++) { buf[k] = '\0'; }

    LCD_Clear();
    LCD_Goto(0U, 0U);
    LCD_PrintPadded(prompt, 16U);
    LCD_Goto(1U, 0U);

    while(1)
    {
        key = Keypad_GetKey();

        if(key == '#')
        {
            if(len == 0U)
            {
                /* Nothing typed yet — ignore */
                continue;
            }

            /* Convert buf to integer */
            value = 0;
            for(k = 0U; k < len; k++)
            {
                value = (signed int)((value * 10) + (buf[k] - '0'));
            }

            if((value >= minVal) && (value <= maxVal))
            {
                *out = value;
                return 1U;
            }

            /* Out of range — show message and let user retry */
            LCD_Print2Lines("Out of range", "Try again");
            delay_ms(1200U);
            LCD_Clear();
            LCD_Goto(0U, 0U);
            LCD_PrintPadded(prompt, 16U);
            LCD_Goto(1U, 0U);
            len    = 0U;
            buf[0] = '\0';
        }
        else if(key == '*')
        {
            return 0U;                  /* cancel */
        }
        else if(key == 'D')
        {
            if(len > 0U)               /* backspace */
            {
                len--;
                buf[len] = '\0';
                LCD_Goto(1U, (u8)len);
                Write_DAT_LCD(' ');
                LCD_Goto(1U, (u8)len);
            }
        }
        else if((key >= '0') && (key <= '9'))
        {
            if(len < ((u8)sizeof(buf) - 1U))
            {
                buf[len++] = key;
                buf[len]   = '\0';
                Write_DAT_LCD(key);
            }
            /* silently ignore if buffer full */
        }
    }
}

/* ---------------------------------------------------------------
 * Sub-menu: Set-points
 *   1 → set temperature alarm threshold
 *   2 → set humidity alarm threshold
 *   * → back
 * --------------------------------------------------------------- */
static void Menu_SetPoints(SystemConfig *cfg)
{
    char       key;
    signed int value;

    while(1)
    {
        LCD_Print2Lines("1:Temperature", "2:Humidity  *:Bk");
        key = Keypad_GetKey();

        if(key == '1')
        {
            if(read_number("Set Temp (0-50C)",
                           TEMP_SETPOINT_MIN, TEMP_SETPOINT_MAX, &value))
            {
                if(EEPROM_SaveTempSetPoint(cfg, (s16)value))
                {
                    LCD_Print2Lines("Temp Setpoint", "Saved to EEPROM");
                }
                else
                {
                    cfg->tempSetPoint = (s16)value;  /* keep in RAM at least */
                    LCD_Print2Lines("Temp Setpoint", "EEPROM Error!");
                }
                delay_ms(1200U);
            }
        }
        else if(key == '2')
        {
            if(read_number("Humidity(20-95%)",
                           HUMIDITY_SETPOINT_MIN, HUMIDITY_SETPOINT_MAX, &value))
            {
                if(EEPROM_SaveHumiditySetPoint(cfg, (u8)value))
                {
                    LCD_Print2Lines("Humidity Setpt", "Saved to EEPROM");
                }
                else
                {
                    cfg->humiditySetPoint = (u8)value;  /* keep in RAM at least */
                    LCD_Print2Lines("Humidity Setpt", "EEPROM Error!");
                }
                delay_ms(1200U);
            }
        }
        else if(key == '*')
        {
            return;
        }
    }
}

/* ---------------------------------------------------------------
 * Sub-menu: Status — show current set-points
 *   Any key → back
 * --------------------------------------------------------------- */
static void Menu_ShowStatus(const SystemConfig *cfg)
{
    LCD_Clear();
    LCD_Goto(0U, 0U);
    Write_str_LCD("T:");
    Write_int_LCD(cfg->tempSetPoint);
    Write_DAT_LCD((char)0xDF);   /* degree symbol on HD44780 */
    Write_DAT_LCD('C');
    Write_str_LCD(" H:");
    Write_int_LCD((signed int)cfg->humiditySetPoint);
    Write_DAT_LCD('%');

    LCD_Goto(1U, 0U);
    Write_str_LCD("PwLen:4  *=Back");

    Keypad_GetKey();   /* wait for any key */
}

/* ---------------------------------------------------------------
 * Main menu
 *   1 → Set-points
 *   2 → Change password
 *   3 → Status
 *   * → Exit
 * --------------------------------------------------------------- */
void Menu_Run(SystemConfig *cfg)
{
    char key;

    while(1)
    {
        LCD_Print2Lines("1:Setpoints 2:Pw", "3:Status  *:Exit");
        key = Keypad_GetKey();

        if(key == '1')
        {
            Menu_SetPoints(cfg);
        }
        else if(key == '2')
        {
            (void)Password_Change(cfg);
        }
        else if(key == '3')
        {
            Menu_ShowStatus(cfg);
        }
        else if(key == '*')
        {
            LCD_Print2Lines("Returning to", "Monitor...");
            delay_ms(700U);
            return;
        }
    }
}
