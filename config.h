#ifndef __COLDGUARD_CONFIG_H__
#define __COLDGUARD_CONFIG_H__

/* -------------------------------------------------------------
 * Clock / UART settings
 * The system clock (CCLK) is configured to 60 MHz by the startup
 * assembly script (Startup.s) using a 12 MHz crystal and the PLL:
 *   PLL_SETUP = 1, PLLCFG = 0x24 (M=5, P=2) → CCLK = 12 MHz × 5 = 60 MHz
 *   MAM fully enabled (MAMCR=2, MAMTIM=4)
 * SystemClock_Init() in main.c sets VPBDIV=0x00 → PCLK = CCLK/4 = 15 MHz.
 * UART divisor below assumes PCLK = 15 MHz.
 * ------------------------------------------------------------- */
#define FOSC                          12000000UL
#define CCLK                          (5UL * FOSC)
#define PCLK                          (CCLK / 4UL)
#define UART_BAUD                     9600UL
#define UART0_DIVISOR                 (PCLK / (16UL * UART_BAUD))

/* -------------------------------------------------------------
 * First EEPROM programming mode
 * Set to 1 ONLY for a true factory reset, then rebuild with 0.
 * WARNING: Leaving this at 1 wipes all user-saved settings (setpoints,
 * password) on EVERY power-on. Always set to 0 for normal operation.
 * ------------------------------------------------------------- */
#define EEPROM_FIRST_TIME_SETUP       0

#define DEFAULT_TEMP_SETPOINT         35     /* Degree Celsius */
#define DEFAULT_HUMIDITY_SETPOINT     65    /* %RH */
#define DEFAULT_PASSWORD              "1234"
#define PASSWORD_MIN_LEN              4
#define PASSWORD_MAX_LEN              4
#define MAX_PASSWORD_ATTEMPTS         3

#define TEMP_SETPOINT_MIN             0
#define TEMP_SETPOINT_MAX             50
#define HUMIDITY_SETPOINT_MIN         20
#define HUMIDITY_SETPOINT_MAX         95

/* -------------------------------------------------------------
 * ThingSpeak / Wi-Fi. Edit these before running ESP01 upload.
 * NOTE: For production / version-controlled code, move WIFI_SSID,
 * WIFI_PASSWORD and THINGSPEAK_WRITE_API_KEY to a separate secrets.h
 * that is listed in .gitignore so credentials are not leaked.
 * ------------------------------------------------------------- */
#define ESP_ENABLE                    1
#define WIFI_SSID                     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD                 "YOUR_WIFI_PASSWORD"
#define THINGSPEAK_WRITE_API_KEY      "YOUR_API_KEY_HERE"
#define THINGSPEAK_HOST               "api.thingspeak.com"
#define THINGSPEAK_PORT               80
#define ESP_RESPONSE_BUFFER_SIZE      300

/* ThingSpeak fields used by this firmware:
 * field1 = temperature, field2 = humidity, field3 = door status,
 * field4 = alarm code: 0 normal, 1 temp, 2 humidity, 3 temp+humidity, 4 door.
 */

#define SENSOR_SAMPLE_DELAY_MS        1000UL
#define DOOR_OPEN_ALERT_SECONDS       15U

/* -------------------------------------------------------------
 * LPC2148 pin assignment used by generated code
 * ------------------------------------------------------------- */
/* UART0 for ESP01 and debug logging */
#define UART0_TXD_PIN                 0     /* P0.0 -> ESP01 RX */
#define UART0_RXD_PIN                 1     /* P0.1 <- ESP01 TX */

/* I2C0 for AT24C256 */
#define I2C0_SCL_PIN                  2     /* P0.2 SCL0 */
#define I2C0_SDA_PIN                  3     /* P0.3 SDA0 */

/* DHT11 */
#define DHT11_DATA_PIN                4     /* P0.4 */

/* Door interrupt switch */
#define DOOR_PIN                      7     /* P0.7 / EINT2 */
#define DOOR_EINT_NUMBER              2
#define DOOR_ACTIVE_LOW               1     /* 1: switch press connects pin to GND */

/* LCD 8-bit mode (connected via patch cords to P0.8..P0.15 for data, P0.16/P0.17 for RS/EN) */
#define LCD_DATA_START_PIN            8     /* P0.8..P0.15 = D0..D7 */
#define LCD_RS_PIN                    16    /* P0.16 */
#define LCD_EN_PIN                    17    /* P0.17 */

/* Buzzer */
#define BUZZER_PIN                    19    /* P0.19 */
#define BUZZER_ACTIVE_HIGH            1

/* Menu/config interrupt switch */
#define MENU_SWITCH_PIN               20    /* P0.20 / EINT3 */
#define MENU_EINT_NUMBER              3
#define MENU_SWITCH_ACTIVE_LOW        1

/* 4x4 keypad on Port 1 (board wiring: ROW=P1.20..23 outputs, COL=P1.16..19 inputs) */
#define KEYPAD_ROW0_PIN               20    /* P1.20 = ROW0 output */
#define KEYPAD_ROW1_PIN               21    /* P1.21 = ROW1 output */
#define KEYPAD_ROW2_PIN               22    /* P1.22 = ROW2 output */
#define KEYPAD_ROW3_PIN               23    /* P1.23 = ROW3 output */
#define KEYPAD_COL0_PIN               16    /* P1.16 = COL0 input  */
#define KEYPAD_COL1_PIN               17    /* P1.17 = COL1 input  */
#define KEYPAD_COL2_PIN               18    /* P1.18 = COL2 input  */
#define KEYPAD_COL3_PIN               19    /* P1.19 = COL3 input  */

#endif
