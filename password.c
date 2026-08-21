#include <string.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"
#include "buzzer.h"
#include "eeprom.h"
#include "password.h"
// #include "logger.h"

static u8 is_digit_key(char key)
{
    if((key >= '0') && (key <= '9'))
    {
        return 1U;
    }
    return 0U;
}

u8 Password_Read(const char *prompt, char *out, u8 maxLen)
{
    u8 len;
    char key;

    len = 0U;
    out[0] = '\0';

    LCD_Clear();
    LCD_Goto(0U, 0U);
    LCD_PrintPadded(prompt, 16U);
    LCD_Goto(1U, 0U);

    while(1)
    {
        key = Keypad_GetKey();

        if(key == '*') /* Cancel */
        {
            out[0] = '\0';
            return 0U;
        }
        else if(key == 'D') /* Backspace */
        {
            if(len > 0U)
            {
                len--;
                out[len] = '\0';
                LCD_Goto(1U, len);
                Write_DAT_LCD(' ');
                LCD_Goto(1U, len);
            }
        }
        else if(is_digit_key(key))
        {
            if(len < maxLen)
            {
                out[len++] = key;
                out[len]   = '\0';
                Write_DAT_LCD('*');

                /* Auto-confirm once the fixed length is reached */
                if(len == maxLen)
                {
                    return 1U;
                }
            }
            /* silently ignore excess digits (cannot happen with fixed length) */
        }
        /* '#' key is ignored — entry is auto-confirmed at maxLen digits */
    }
}


u8 Password_Verify(const SystemConfig *cfg)
{
    char entered[PASSWORD_MAX_LEN + 1];
    u8 attempt;

    for(attempt = 0U; attempt < MAX_PASSWORD_ATTEMPTS; attempt++)
    {
        if(!Password_Read("Enter Password", entered, PASSWORD_MAX_LEN))
        {
            LCD_Print2Lines("Cancelled", "Returning...");
            delay_ms(700U);
            return 0U;
        }

        if(strcmp(entered, cfg->password) == 0)
        {
            LCD_Print2Lines("Access Granted", "Welcome");
            Buzzer_Beep(1U, 80U, 80U);
            // Logger_LogEvent(EVENT_ACCESS_GRANTED);
            delay_ms(800U);
            return 1U;
        }

        LCD_Print2Lines("Wrong Password", "Try Again");
        Buzzer_Beep(2U, 100U, 100U);
        delay_ms(1000U);
    }

    LCD_Print2Lines("Access Denied", "Alarm");
    Buzzer_Beep(5U, 100U, 100U);
    // Logger_LogEvent(EVENT_ACCESS_DENIED);
    delay_ms(1000U);
    return 0U;
}

u8 Password_Change(SystemConfig *cfg)
{
    char current[PASSWORD_MAX_LEN + 1];
    char newPass[PASSWORD_MAX_LEN + 1];
    char confirmPass[PASSWORD_MAX_LEN + 1];

    if(!Password_Read("Current Pass", current, PASSWORD_MAX_LEN))
    {
        return 0U;
    }

    if(strcmp(current, cfg->password) != 0)
    {
        LCD_Print2Lines("Wrong Current", "Not Updated");
        Buzzer_Beep(2U, 100U, 100U);
        delay_ms(1200U);
        return 0U;
    }

    if(!Password_Read("New Pass", newPass, PASSWORD_MAX_LEN))
    {
        return 0U;
    }

    if(!Password_Read("Confirm Pass", confirmPass, PASSWORD_MAX_LEN))
    {
        return 0U;
    }

    if(strcmp(newPass, confirmPass) != 0)
    {
        LCD_Print2Lines("Mismatch", "Not Updated");
        Buzzer_Beep(2U, 100U, 100U);
        delay_ms(1200U);
        return 0U;
    }

    {
        if(EEPROM_SavePassword(cfg, newPass))
        {
            LCD_Print2Lines("Password", "Saved to EEPROM");
        }
        else
        {
            /* EEPROM write failed — update RAM so it works this session */
            u8 k;
            for(k = 0U; k <= PASSWORD_MAX_LEN; k++) { cfg->password[k] = '\0'; }
            strncpy(cfg->password, newPass, PASSWORD_MAX_LEN);
            cfg->password[PASSWORD_MAX_LEN] = '\0';
            cfg->passwordLen = (u8)strlen(newPass);
            LCD_Print2Lines("Password", "EEPROM Error!");
        }
        // Logger_LogEvent(EVENT_PASSWORD_CHANGED);
        delay_ms(1200U);
        return 1U;
    }
}
