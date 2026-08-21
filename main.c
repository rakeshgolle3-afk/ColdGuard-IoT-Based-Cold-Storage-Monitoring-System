#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "lcd.h"
#include "dht11.h"
#include "keypad.h"
#include "buzzer.h"
#include "door_interrupt.h"
#include "password.h"
#include "menu.h"
#include "app_config.h"
#include "eeprom.h"
#include "uart0.h"
#include "esp01.h"

/* ---------------------------------------------------------------
 * Global runtime state
 * --------------------------------------------------------------- */
static SystemConfig g_config;

extern volatile u8 g_menuEvent;   /* set by EINT3 ISR in door_interrupt.c */

/* ---------------------------------------------------------------
 * Clock init — VPBDIV = 0x00 → PCLK = CCLK/4 = 15 MHz
 * PLL already running at 60 MHz from Startup.s
 * --------------------------------------------------------------- */
static void SystemClock_Init(void)
{
    VPBDIV = 0x00U;
}

/* Config_SetDefaults() removed — EEPROM_LoadConfig() now handles
 * both loading saved settings and recovering safe defaults on a
 * blank / corrupted EEPROM (see Init_All). */

/* ---------------------------------------------------------------
 * DigitWidth — printed character width of a signed integer
 *   0  → 1,   5  → 1,  24 → 2,  -5 → 2,  -15 → 3
 * --------------------------------------------------------------- */
static int DigitWidth(signed int n)
{
    int w;
    if(n == 0) { return 1; }
    w = (n < 0) ? 1 : 0;       /* leading minus counts as one char */
    if(n < 0)  { n = -n; }
    while(n > 0) { n /= 10; w++; }
    return w;
}

/* ---------------------------------------------------------------
 * DisplayMonitorScreen
 *
 * Line 1 — full-width justified across all 16 columns:
 *   LEFT  "T:XX°C"   right-padded with dynamic spaces
 *   RIGHT "RH:XX%"   flush to column 15
 *
 *   Example layouts (16 chars each):
 *     "T:24°C    RH:58%"   (2-digit temp, 2-digit humidity)
 *     "T:5°C     RH:58%"   (1-digit temp)
 *     "T:-5°C    RH:58%"   (negative temp)
 *
 * Line 2 (max 16 chars):
 *   "Door:Closed     "  (16 chars, space-padded)
 *   "Door:Open T Xs  "  (countdown seconds until 15s alert)
 * --------------------------------------------------------------- */
static void DisplayMonitorScreen(signed int temp, unsigned int humidity,
                                 u32 doorOpenMs)
{
    int leftLen, rightLen, spaces, i;

    /* Overwrite in-place — no LCD_Clear() so no visible flicker */

    /* Line 1: left = "T:XX°C", right = "RH:XX%", gap fills to 16 cols */
    leftLen  = 4 + DigitWidth(temp);         /* 'T'':' + digits + '°''C' */
    rightLen = 4 + ((humidity < 10U) ? 1 :   /* 'R''H'':' + digits + '%' */
                    (humidity < 100U) ? 2 : 3);
    spaces   = 16 - leftLen - rightLen;
    if(spaces < 1) { spaces = 1; }           /* safety: never 0 or negative */

    LCD_Goto(0U, 0U);
    Write_str_LCD("T:");
    Write_int_LCD(temp);
    Write_DAT_LCD((char)0xDF);               /* HD44780 degree symbol */
    Write_DAT_LCD('C');
    for(i = 0; i < spaces; i++) { Write_DAT_LCD(' '); }
    Write_str_LCD("RH:");
    Write_int_LCD((signed int)humidity);
    Write_DAT_LCD('%');

    /* Line 2: door status — show elapsed seconds when open so there
     * is never a bare "Door:Open" flash before HandleDoorLogic writes
     * the timer.  Always exactly 16 chars to overwrite any stale text. */
    LCD_Goto(1U, 0U);
    if(Door_IsOpen())
    {
        u32 elapsedSec = doorOpenMs / 1000U;
		signed int remainingSec = (signed int)DOOR_OPEN_ALERT_SECONDS - (signed int)elapsedSec;
		if(remainingSec<0){
			remainingSec=0;
		}												   
        Write_str_LCD("Door:Open T ");
        Write_int_LCD(remainingSec);
        Write_str_LCD("s   ");               /* trailing spaces clear stale digits */
    }
    else
    {
        Write_str_LCD("Door:Closed     ");
    }
}

/* ---------------------------------------------------------------
 * HandleSensorAlarms
 *
 * Checks if temperature or humidity exceeds the configured setpoint.
 * Returns alarmCode bitmask:
 *   bit 0 = temperature alarm
 *   bit 1 = humidity alarm
 *
 * NOTE: No LCD output here — line 2 always shows door status.
 * The alarmCode is used for the buzzer and future ESP01 reporting.
 * --------------------------------------------------------------- */
static u8 HandleSensorAlarms(signed int temp, unsigned int humidity)
{
    u8 alarmCode = 0U;

    if(temp     > (signed int)  g_config.tempSetPoint)    { alarmCode |= 0x01U; }
    if(humidity > (unsigned int)g_config.humiditySetPoint){ alarmCode |= 0x02U; }

    /* Trigger buzzer if any threshold is exceeded */
    if(alarmCode != 0U)
    {
        Buzzer_On();   /* activate buzzer — threshold exceeded */
    }
    else
    {
        Buzzer_Off();
    }

    return alarmCode;
}

/* ---------------------------------------------------------------
 * HandleDoorLogic
 *
 * Project requirement:
 *   - Track door-open time with a countdown timer on LCD
 *   - If open > 15 s: show alert on LCD, buzzer ON
 *   - If door closes after > 15 s: send TWO updates to ThingSpeak:
 *       1) field3 = 1  ("Door Open"  event timestamp)
 *       2) field3 = 0  ("Door Closed" event timestamp, after 16 s gap)
 *     ThingSpeak can then calculate open-duration = timestamp2 - timestamp1.
 *   - If door closes within 30 s: no ThingSpeak update
 *
 * Time is tracked using the actual elapsed milliseconds passed in
 * from the main loop, not a fixed increment, so display delays
 * (LCD_Print2Lines calls) do not drift the count.
 * --------------------------------------------------------------- */
static u8 HandleDoorLogic(u32 cycleMs, u32 *pDoorOpenMs)
{
    static u32 doorOpenMs = 0U;
    static u8  alertShown = 0U;
    static u8  firstOpen  = 1U;          /* flag for first-open message */

    *pDoorOpenMs = doorOpenMs;           /* expose to caller before any update */

    if(Door_IsOpen())
    {
        doorOpenMs += cycleMs;

        if(firstOpen)                       /* first cycle door is open */
        {
            firstOpen = 0U;
            LCD_Print2Lines("Door Opened", "Timer started");
            delay_ms(600U);
        }

        if(doorOpenMs >= ((u32)DOOR_OPEN_ALERT_SECONDS * 1000U))
        {
            /* Door has been open past the 15-second threshold */
            LCD_Goto(1U, 0U);
            Write_str_LCD("DOOR >15s ALERT ");

            if(!alertShown)
            {
                alertShown = 1U;
            }

            Buzzer_On();                    /* keep buzzing while door stays open past limit */
        }
        else
        {
            /* DisplayMonitorScreen already wrote "Door:Open T Xs" —
             * nothing extra needed here while under the threshold. */
        }
    }
    else
    {
        if(doorOpenMs != 0U)
        {
            /* Door was open and is now closing */
            if(doorOpenMs >= ((u32)DOOR_OPEN_ALERT_SECONDS * 1000U))
            {
                /* Exceeded 15s — send "door was open" alert to ThingSpeak.
                 * Only field3=1 (open) is sent here; the periodic 1-minute
                 * ESP01_SendUpdate() will report field3=0 (closed) on its
                 * next cycle, so there is no need to block 16 s for a
                 * second CIPSEND. */
                LCD_Print2Lines("Door Closed", "Sending status..");
                delay_ms(500U);

                /* Send field3=1 → marks door-OPEN event on ThingSpeak */
                ESP01_SendField(3U, 1);

                /* ThingSpeak requires ~15 s between updates.
                 * Show a countdown so the user can see progress. */
                LCD_Print2Lines("Open sent", "Waiting 16s...");
                delay_ms(16000U);

                /* Send field3=0 → marks door-CLOSED event on ThingSpeak.
                 * The time difference between the two entries gives the
                 * exact duration the door was open. */
                ESP01_SendField(3U, 0);

                LCD_Print2Lines("Status Sent", "Monitoring...");
                delay_ms(700U);
                doorOpenMs = 0U;
                alertShown = 0U;
                firstOpen  = 1U;                    /* reset for next open event */

                Buzzer_Off();
                return 1U;                  /* tell main() we sent ThingSpeak data */
            }
            else
            {
                /* Closed within 15s — no ThingSpeak update needed */
                LCD_Print2Lines("Door Closed", "Monitoring...");
                delay_ms(700U);
            }
        }
        doorOpenMs = 0U;
        alertShown = 0U;
        firstOpen  = 1U;                    /* reset for next open event */

        Buzzer_Off();                       /* stop buzzer when door closes */
    }
    return 0U;
}

/* ---------------------------------------------------------------
 * HandleMenuRequest — password gate to menu
 * --------------------------------------------------------------- */
static void HandleMenuRequest(void)
{
    delay_ms(30U);   /* debounce */
    if(Password_Verify(&g_config))
    {
        Menu_Run(&g_config);
    }
	delay_ms(30U);
	(void)Menu_GetAndClearEvent();
}

/* ---------------------------------------------------------------
 * Init_All — startup sequence
 * Order matters: Timer0 first (delay depends on it),
 *                LCD second (gives visual feedback immediately),
 *                then sensors and interrupts.
 * --------------------------------------------------------------- */
static void Init_All(void)
{
    SystemClock_Init();
    Timer0_Init();                         /* delay_us/ms need this first */
    UART0_Init();

    LCD_Init();
    LCD_Print2Lines("ColdGuard", "Initialising...");
    delay_ms(800U);

    Buzzer_Init();

    Keypad_Init();

    /* I2C0 + AT24C256 EEPROM (P0.2=SCL, P0.3=SDA) */
    EEPROM_Init();
    if(EEPROM_LoadConfig(&g_config))
    {
        LCD_Print2Lines("Config Loaded", "From EEPROM");
    }
    else
    {
        LCD_Print2Lines("Config Defaults", "Loaded");
    }
    delay_ms(700U);

    DHT11_Init();                          /* 1200 ms stabilisation inside */
    LCD_Print2Lines("DHT11 Sensor", "Ready");
    delay_ms(400U);

    ESP01_Init();

    DoorInterrupt_Init();
    LCD_Print2Lines("ColdGuard", "Running...");
    delay_ms(600U);
}

/* ---------------------------------------------------------------
 * main
 * --------------------------------------------------------------- */
int main(void)
{
    DHT11_Data   sensor;
    signed int   temp;
    unsigned int humidity;
    u8           dhtFailCount;
    u8           dhtHasData;
    u8           alarmCode;
    u32          elapsed;
    u32          thingSpeakTimer;
    u32          doorOpenMs;

    Init_All();

    temp         = DEFAULT_TEMP_SETPOINT;
    humidity     = DEFAULT_HUMIDITY_SETPOINT;
    dhtFailCount = 0U;
    dhtHasData   = 0U;
    thingSpeakTimer = 60000U; /* trigger immediately on boot */
    doorOpenMs      = 0U;

    while(1)
    {
        /* --------------------------------------------------------
         * 1. Menu interrupt check (EINT3 switch)
         * -------------------------------------------------------- */
        if(Menu_GetAndClearEvent())
        {
            HandleMenuRequest();
            continue;                      /* restart loop after menu exits */
        }

        /* --------------------------------------------------------
         * 2. Keypad poll (keeps buffer clear, menu uses interrupt)
         * -------------------------------------------------------- */
        (void)Keypad_GetKeyNonBlocking();

        /* --------------------------------------------------------
         * 3. DHT11 read
         *    Success  → update temp/humidity, reset fail counter
         *    Fail ≤3  → show "Retrying" and wait 1 s before next
         *    Fail >3  → use last known value so other functions
         *                (door logic, alarm) keep running
         * -------------------------------------------------------- */
        if(DHT11_Read(&sensor))
        {
            temp         = (signed int)sensor.temp_integer;
            humidity     = (unsigned int)sensor.humidity_integer;
            dhtFailCount = 0U;
            dhtHasData   = 1U;
        }
        else
        {
            dhtFailCount++;

            if(dhtFailCount <= 3U)
            {
                LCD_Print2Lines("DHT11 Error", "Retrying...");
                delay_ms(1000U);   /* DHT11 needs >=1 s before retry */
                continue;          /* skip display/alarm this cycle    */
            }
            else
            {
                /* More than 3 consecutive failures — keep running    */
                LCD_Goto(0U, 0U);
                if(dhtHasData)
                {
                    Write_str_LCD("DHT11 Err(last) ");
                }
                else
                {
                    Write_str_LCD("DHT11 Err(dflt) ");
                }
            }
        }

        /* --------------------------------------------------------
         * 4. Display monitor screen (line1=sensor values, line2=door)
         * -------------------------------------------------------- */
        /* doorOpenMs must be read BEFORE HandleDoorLogic updates it,
         * so DisplayMonitorScreen shows the value from the previous
         * cycle — accurate to one cycle (≤1 s), no flash. */
        DisplayMonitorScreen(temp, humidity, doorOpenMs);

        /* --------------------------------------------------------
         * 5. Alarm check — activates buzzer if threshold exceeded
         * -------------------------------------------------------- */
        alarmCode = HandleSensorAlarms(temp, humidity);

        /* --------------------------------------------------------
         * 6. Door logic — overwrites line2 if door events occur
         *    Pass actual cycle time (SENSOR_SAMPLE_DELAY_MS = 1000ms)
         *    so the timer stays accurate regardless of loop delays
         * -------------------------------------------------------- */
        if(HandleDoorLogic((u32)SENSOR_SAMPLE_DELAY_MS, &doorOpenMs))
        {
            /* Door logic just sent ThingSpeak data — reset periodic
             * timer to avoid rate-limit rejection (ThingSpeak needs
             * ~15 s between updates). */
            thingSpeakTimer = 0U;
        }

        /* --------------------------------------------------------
         * 7. ThingSpeak Upload (every 60 seconds / 1 minute)
         * -------------------------------------------------------- */
        thingSpeakTimer += SENSOR_SAMPLE_DELAY_MS;
        if(thingSpeakTimer >= 60000U)
        {
            ESP01_SendUpdate(temp, humidity, Door_IsOpen(), alarmCode);
            thingSpeakTimer = 0U;
        }

        /* --------------------------------------------------------
         * 8. Wait 1 s between samples, but wake early on menu event
         * -------------------------------------------------------- */
        elapsed = 0U;
        while((elapsed < SENSOR_SAMPLE_DELAY_MS) && (g_menuEvent == 0U))
        {
            delay_ms(50U);
            elapsed += 50U;
        }
    }
}
