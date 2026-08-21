#include <string.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "uart0.h"
#include "lcd.h"
#include "esp01.h"

/* ---------------------------------------------------------------
 * buff[], i are declared and filled by uart0.c ISR.
 * We reset i=0 and memset(buff,0) before each command, then
 * delay and inspect with strstr — exactly the working reference.
 * --------------------------------------------------------------- */
extern char                  buff[];
extern volatile unsigned int i;

/* ---------------------------------------------------------------
 * ESP01_GetLastResponse — copy buff into caller's buffer
 * --------------------------------------------------------------- */
void ESP01_GetLastResponse(char *dest, u16 maxLen)
{
    u16 k;
    if(maxLen == 0U) return;
    for(k = 0U; (k < (u16)(maxLen - 1U)) && (buff[k] != '\0'); k++)
        dest[k] = buff[k];
    dest[k] = '\0';
}

/* ---------------------------------------------------------------
 * int_to_str — convert signed int to decimal ASCII string.
 * Returns pointer to buf for convenient use with strcat().
 * --------------------------------------------------------------- */
static char* int_to_str(signed int val, char *buf, u8 bufSize)
{
    u8  k = 0U;
    u8  neg = 0U;
    unsigned int uv;
    char tmp;
    u8  start, end;

    if(val < 0) { neg = 1U; uv = (unsigned int)(-(val + 1)) + 1U; }
    else        { uv  = (unsigned int)val; }

    if(uv == 0U) { buf[k++] = '0'; }
    while((uv > 0U) && (k < (u8)(bufSize - 1U)))
    {
        buf[k++] = (char)((uv % 10U) + '0');
        uv /= 10U;
    }
    if(neg && (k < (u8)(bufSize - 1U))) { buf[k++] = '-'; }
    buf[k] = '\0';

    /* Reverse in-place */
    if(k > 0U)
    {
        start = 0U; end = (u8)(k - 1U);
        while(start < end)
        {
            tmp = buf[start]; buf[start] = buf[end]; buf[end] = tmp;
            start++; end--;
        }
    }
    return buf;
}

/* ===============================================================
 * ESP01_Init / esp01_connectAP
 *
 * Follows the working reference step-by-step:
 *   AT  →  ATE0  →  CWMODE=1  →  CIPMUX=0  →  CWQAP  →  CWJAP
 * WiFi credentials come from config.h (WIFI_SSID / WIFI_PASSWORD).
 * =============================================================== */
u8 ESP01_Init(void)
{
    char cmd[120];
    u8   ok;

    /* --- AT check ------------------------------------------------ */
    LCD_Print2Lines("ESP CHECK", "");
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)"AT\r\n");
    delay_ms(1000U);

    if(strstr(buff, "OK"))
        LCD_Print2Lines("ESP READY", "");
    else
        LCD_Print2Lines("ESP ERROR", "");
    delay_ms(1000U);

    /* --- Echo off ----------------------------------------------- */
    LCD_Print2Lines("ECHO OFF", "");
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)"ATE0\r\n");
    delay_ms(500U);

    /* --- Station mode ------------------------------------------- */
    LCD_Print2Lines("WIFI MODE", "");
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)"AT+CWMODE=1\r\n");
    delay_ms(1000U);

    /* --- Single connection -------------------------------------- */
    LCD_Print2Lines("NET CONFIG", "");
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)"AT+CIPMUX=0\r\n");
    delay_ms(500U);

    /* --- Disconnect old AP -------------------------------------- */
    LCD_Print2Lines("RESET WIFI", "");
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)"AT+CWQAP\r\n");
    delay_ms(1000U);

    /* --- Join AP (credentials from config.h) ------------------- */
    LCD_Print2Lines("CONNECT WIFI", WIFI_SSID);
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    strcpy(cmd, "AT+CWJAP=\"");
    strcat(cmd, WIFI_SSID);
    strcat(cmd, "\",\"");
    strcat(cmd, WIFI_PASSWORD);
    strcat(cmd, "\"\r\n");
    uart0_str((u8 *)cmd);
    delay_ms(12000U);

    ok = (strstr(buff, "WIFI CONNECTED") != 0) ||
         (strstr(buff, "WIFI GOT IP")    != 0) ||
         (strstr(buff, "OK")             != 0);

    if(ok)
    {
        LCD_Print2Lines("WIFI CONNECTED", "ESP01 Ready");
        delay_ms(2000U);
        return 1U;
    }

    LCD_Print2Lines("WIFI FAILED", "Check config.h");
    delay_ms(2000U);
    return 0U;
}

void esp01_connectAP(void)
{
    (void)ESP01_Init();
}

/* ===============================================================
 * Internal helper — open TCP, send HTTP GET, close.
 * request  : the full GET line (e.g. "GET /update?api_key=...&field1=25\r\n\r\n")
 * sendLen  : value to pass to AT+CIPSEND (must match strlen(request))
 * =============================================================== */
static u8 SendHTTPGet(const char *request, unsigned int sendLen)
{
    char lenStr[8];
    char cmd[24];

    /* Build AT+CIPSEND=<len> string using our helper */
    int_to_str((signed int)sendLen, lenStr, (u8)sizeof(lenStr));

    /* Close any stale TCP connection */
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)"AT+CIPCLOSE\r\n");
    delay_ms(500U);

    /* Open TCP to ThingSpeak */
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)"AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n");
    delay_ms(3000U);

    if(!strstr(buff, "CONNECT"))
    {
        LCD_Print2Lines("TCP FAILED", "");
        delay_ms(2000U);
        return 0U;
    }

    /* AT+CIPSEND=<len> */
    strcpy(cmd, "AT+CIPSEND=");
    strcat(cmd, lenStr);
    strcat(cmd, "\r\n");
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)cmd);
    delay_ms(500U);

    /* Send the HTTP GET — 5 s is enough for ThingSpeak (typical < 3 s) */
    i = 0; memset(buff, 0, ESP_RESPONSE_BUFFER_SIZE);
    uart0_str((u8 *)request);
    delay_ms(5000U);

    /* Make sure buff is null-terminated before strstr */
    if(i < (unsigned int)(ESP_RESPONSE_BUFFER_SIZE - 1U))
        buff[i] = '\0';
    else
        buff[ESP_RESPONSE_BUFFER_SIZE - 1U] = '\0';

    if(strstr(buff, "SEND OK"))
    {
        LCD_Print2Lines("DATA SENT OK", "");
        delay_ms(500U);
        uart0_str((u8 *)"AT+CIPCLOSE\r\n");
        delay_ms(500U);
        return 1U;
    }

    LCD_Print2Lines("SEND FAILED", "");
    delay_ms(500U);
    uart0_str((u8 *)"AT+CIPCLOSE\r\n");
    delay_ms(500U);
    return 0U;
}

/* ===============================================================
 * ESP01_SendUpdate — send all 4 project fields to ThingSpeak
 *
 * Fields:
 *   field1 = temperature
 *   field2 = humidity
 *   field3 = door open (0/1)
 *   field4 = alarm code
 * =============================================================== */
u8 ESP01_SendUpdate(signed int temperature, unsigned int humidity,
                    unsigned int doorOpen,  unsigned int alarmCode)
{
    char request[180];
    char num[8];
    unsigned int len;

    LCD_Print2Lines("SENDING DATA", "");

    strcpy(request, "GET /update?api_key=");
    strcat(request, THINGSPEAK_WRITE_API_KEY);

    strcat(request, "&field1=");
    strcat(request, int_to_str(temperature, num, (u8)sizeof(num)));

    strcat(request, "&field2=");
    strcat(request, int_to_str((signed int)humidity, num, (u8)sizeof(num)));

    strcat(request, "&field3=");
    strcat(request, doorOpen ? "1" : "0");

    strcat(request, "&field4=");
    strcat(request, int_to_str((signed int)alarmCode, num, (u8)sizeof(num)));

    strcat(request, "\r\n\r\n");

    len = (unsigned int)strlen(request);
    return SendHTTPGet(request, len);
}

/* ===============================================================
 * ESP01_SendField — send a single field (used by esp01_sendToThingspeak)
 * =============================================================== */
u8 ESP01_SendField(u8 fieldNo, signed int value)
{
    char request[140];
    char num[8];
    unsigned int len;

    strcpy(request, "GET /update?api_key=");
    strcat(request, THINGSPEAK_WRITE_API_KEY);
    strcat(request, "&field");

    /* field number (1 digit) */
    { char c[2]; c[0] = (char)('0' + (int)fieldNo); c[1] = '\0'; strcat(request, c); }

    strcat(request, "=");
    strcat(request, int_to_str(value, num, (u8)sizeof(num)));

    strcat(request, "\r\n\r\n");
    len = (unsigned int)strlen(request);
    return SendHTTPGet(request, len);
}

/* ---------------------------------------------------------------
 * esp01_sendToThingspeak — compatibility wrapper (field1)
 * --------------------------------------------------------------- */
void esp01_sendToThingspeak(char *val)
{
    signed int number = 0;
    char *p = val;
    while((*p >= '0') && (*p <= '9'))
    {
        number = (signed int)((number * 10) + (*p - '0'));
        p++;
    }
    (void)ESP01_SendField(1U, number);
}
