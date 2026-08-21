#include <LPC214x.h>
#include "types.h"
#include "config.h"
#include "delay.h"
#include "keypad.h"

#define ROW_MASK ((1UL << KEYPAD_ROW0_PIN) | (1UL << KEYPAD_ROW1_PIN) | (1UL << KEYPAD_ROW2_PIN) | (1UL << KEYPAD_ROW3_PIN))
#define COL_MASK ((1UL << KEYPAD_COL0_PIN) | (1UL << KEYPAD_COL1_PIN) | (1UL << KEYPAD_COL2_PIN) | (1UL << KEYPAD_COL3_PIN))

static const char keyMap[4][4] =
{
    {'1','2','3','A'},  /* Row 0 (P1.20) -> 1, 2, 3, A */
    {'4','5','6','B'},  /* Row 1 (P1.21) -> 4, 5, 6, B */
    {'7','8','9','C'},  /* Row 2 (P1.22) -> 7, 8, 9, C */
    {'*','0','#','D'}   /* Row 3 (P1.23) -> *, 0, #, D */
};

static u8 rowPins[4] = {KEYPAD_ROW0_PIN, KEYPAD_ROW1_PIN, KEYPAD_ROW2_PIN, KEYPAD_ROW3_PIN};
static u8 colPins[4] = {KEYPAD_COL0_PIN, KEYPAD_COL1_PIN, KEYPAD_COL2_PIN, KEYPAD_COL3_PIN};

void Keypad_Init(void)
{
    /* Clear only the P1.16..P1.23 GPIO function bits in PINSEL2.
     * Bits 2..17 correspond to P1.16..P1.23. Clearing them configures
     * P1.16..P1.23 as GPIO for the keypad. */
    PINSEL2 &= ~0x0003FFFCUL;   /* Clear bits 2..17 (P1.16..P1.23 as GPIO); preserves debug/trace control */

    /* ROW pins (P1.20..P1.23) = OUTPUTS, driven low one-at-a-time during scan */
    IODIR1 |= ROW_MASK;
    /* COL pins (P1.16..P1.19) = INPUTS, pulled high externally; read for key detection */
    IODIR1 &= ~COL_MASK;
    /* Idle state: all rows HIGH (deasserted) */
    IOSET1 = ROW_MASK;
}

/* NOTE: Despite its name, Keypad_GetKeyNonBlocking() blocks briefly while a key
 * is physically held down (it waits for key-release before returning the key
 * character). It is non-blocking only in the sense that it returns 0 immediately
 * when NO key is pressed at all.  The main loop therefore stalls for the duration
 * of a key-hold; for typical brief presses this is negligible. */
char Keypad_GetKeyNonBlocking(void)
{
    u8 r;
    u8 c;
    u32 rowMask;
    u32 colMask;

    for(r = 0U; r < 4U; r++)
    {
        /* Assert one row LOW at a time; all other rows stay HIGH */
        IOSET1 = ROW_MASK;
        rowMask = (1UL << rowPins[r]);
        IOCLR1 = rowMask;
        delay_us(10U);

        for(c = 0U; c < 4U; c++)
        {
            colMask = (1UL << colPins[c]);
            if((IOPIN1 & colMask) == 0U) /* col pulled LOW by pressed key */
            {
                delay_ms(20U);  /* debounce */
                if((IOPIN1 & colMask) == 0U)
                {
                    /* Wait for key release */
                    while((IOPIN1 & colMask) == 0U);
                    IOSET1 = ROW_MASK; /* return all rows idle HIGH */
                    return keyMap[r][c];
                }
            }
        }
    }

    IOSET1 = ROW_MASK; /* ensure rows idle HIGH before returning */
    return 0;
}

char Keypad_GetKey(void)
{
    char key;

    key = 0;
    while(key == 0)
    {
        key = Keypad_GetKeyNonBlocking();
    }
    return key;
}
