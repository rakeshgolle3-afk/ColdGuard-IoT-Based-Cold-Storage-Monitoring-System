#ifndef __DEFINES_H__
#define __DEFINES_H__

#define SETBIT(WORD,BITPOS)             ((WORD) |=  (1UL << (BITPOS)))
#define CLRBIT(WORD,BITPOS)             ((WORD) &= ~(1UL << (BITPOS)))
#define CPLBIT(WORD,BITPOS)             ((WORD) ^=  (1UL << (BITPOS)))
#define WRITEBIT(WORD,BITPOS,BIT)       ((BIT) ? SETBIT((WORD),(BITPOS)) : CLRBIT((WORD),(BITPOS)))
#define READBIT(WORD,BITPOS)            (((WORD) >> (BITPOS)) & 1U)
#define WRITENIBBLE(WORD,BITPOS,NIBBLE) ((WORD) = (((WORD) & ~(0x0FUL << (BITPOS))) | (((u32)(NIBBLE) & 0x0F) << (BITPOS))))
#define READNIBBLE(WORD,BITPOS)         (((WORD) >> (BITPOS)) & 0x0FU)
#define WRITEBYTE(WORD,BITPOS,BYTE)     ((WORD) = (((WORD) & ~(0xFFUL << (BITPOS))) | (((u32)(BYTE) & 0xFF) << (BITPOS))))
#define READBYTE(WORD,BITPOS)           (((WORD) >> (BITPOS)) & 0xFFU)

/* LPC214x has write-only SET/CLR registers (IOSET, IOCLR).
 * Both accept the same bitmask: writing (1<<BITPOS) to IOSET sets the pin,
 * writing (1<<BITPOS) to IOCLR clears the pin.
 * SSETBIT and SCLRBIT therefore share the same macro body on purpose --
 * the distinction is which REGISTER you pass as WORD.
 * Usage:  SSETBIT(IOSET0, 5)  --> sets   P0.5
 *         SCLRBIT(IOCLR0, 5)  --> clears P0.5
 */
#define SSETBIT(WORD,BITPOS)            ((WORD) = (1UL << (BITPOS)))
#define SCLRBIT(WORD,BITPOS)            ((WORD) = (1UL << (BITPOS)))

#endif
