#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "door_interrupt.h"

volatile u8 g_doorOpenEvent = 0U;
volatile u8 g_menuEvent = 0U;

void EINT2_ISR(void) __irq
{
    g_doorOpenEvent = 1U;
    EXTINT = (1UL << DOOR_EINT_NUMBER);
    VICVectAddr = 0x00UL;
}

void EINT3_ISR(void) __irq
{
    g_menuEvent = 1U;
    EXTINT = (1UL << MENU_EINT_NUMBER);
    VICVectAddr = 0x00UL;
}

void DoorInterrupt_Init(void)
{
    /* P0.7 as EINT2: PINSEL0 bits 14:15 = 11 */
    PINSEL0 &= ~(3UL << (DOOR_PIN * 2U));
    PINSEL0 |=  (3UL << (DOOR_PIN * 2U));

    /* P0.20 as EINT3: PINSEL1 bits 8:9 = 11 */
    PINSEL1 &= ~(3UL << ((MENU_SWITCH_PIN - 16U) * 2U));
    PINSEL1 |=  (3UL << ((MENU_SWITCH_PIN - 16U) * 2U));

    EXTINT = (1UL << DOOR_EINT_NUMBER) | (1UL << MENU_EINT_NUMBER);
    EXTMODE |= (1UL << DOOR_EINT_NUMBER) | (1UL << MENU_EINT_NUMBER); /* edge triggered */

#if DOOR_ACTIVE_LOW
    EXTPOLAR &= ~(1UL << DOOR_EINT_NUMBER); /* falling edge */
#else
    EXTPOLAR |=  (1UL << DOOR_EINT_NUMBER); /* rising edge */
#endif

#if MENU_SWITCH_ACTIVE_LOW
    EXTPOLAR &= ~(1UL << MENU_EINT_NUMBER); /* falling edge */
#else
    EXTPOLAR |=  (1UL << MENU_EINT_NUMBER); /* rising edge */
#endif

    VICIntSelect &= ~((1UL << 16U) | (1UL << 17U)); /* IRQ */

    /* VIC slot allocation -- update this comment if you add more IRQ handlers:
     *   Slot 0: (reserved / available)
     *   Slot 1: EINT2 (door switch,  IRQ source 16)
     *   Slot 2: EINT3 (menu switch,  IRQ source 17)
     *   Slot 3..15: available
     * If another driver tries to use Slot 1 or 2 it will silently overwrite
     * these vectors, so keep this table up to date. */
    VICVectAddr1 = (unsigned long)EINT2_ISR;
    VICVectCntl1 = 0x20U | 16U;

    VICVectAddr2 = (unsigned long)EINT3_ISR;
    VICVectCntl2 = 0x20U | 17U;

    VICIntEnable = (1UL << 16U) | (1UL << 17U);
}

u8 Door_IsOpen(void)
{
    u8 pinHigh;

    pinHigh = (u8)((IOPIN0 & (1UL << DOOR_PIN)) ? 1U : 0U);
#if DOOR_ACTIVE_LOW
    return (u8)(pinHigh ? 0U : 1U);
#else
    return pinHigh;
#endif
}

u8 MenuSwitch_IsPressed(void)
{
    u8 pinHigh;

    pinHigh = (u8)((IOPIN0 & (1UL << MENU_SWITCH_PIN)) ? 1U : 0U);
#if MENU_SWITCH_ACTIVE_LOW
    return (u8)(pinHigh ? 0U : 1U);
#else
    return pinHigh;
#endif
}

u8 Door_GetAndClearOpenEvent(void)
{
    if(g_doorOpenEvent)
    {
        g_doorOpenEvent = 0U;
        return 1U;
    }
    return 0U;
}

u8 Menu_GetAndClearEvent(void)
{
    if(g_menuEvent)
    {
        g_menuEvent = 0U;
        return 1U;
    }
    return 0U;
}
