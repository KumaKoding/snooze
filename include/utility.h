#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>

#define LE_HBYTE16(u16) (uint8_t)((u16 & 0xFF00) >> 8)
#define LE_LBYTE16(u16) (uint8_t)(u16 & 0x00FF)

#define SWP_LE_LBYTE16(u16, u8) ((u16 & 0xFF00) | u8)
#define SWP_LE_HBYTE16(u16, u8) ((u16 & 0x00FF) | ((0x0000 | u8) << 8))

#define LE_COMBINE_BANK_SHORT(bank, us) (uint32_t)(((((0x00000000 | bank) << 8) | ((us & 0xFF00) >> 8)) << 8) | (us & 0x00FF))
#define LE_COMBINE_BANK_2BYTE(bank, b1, b2) (uint32_t)(((((0x00000000 | bank) << 8) | b2) << 8) | b1)
#define LE_COMBINE_2BYTE(b1, b2) (uint16_t)(((0x0000 | b2) << 8) | b1)
#define LE_COMBINE_3BYTE(b1, b2, b3) (uint32_t)(((((0x00000000 | b3) << 8) | b2) << 8) | b1)

uint8_t check_bit8(uint8_t ps, uint8_t mask);
uint8_t check_bit16(uint16_t ps, uint16_t mask);
uint8_t check_bit32(uint32_t ps, uint32_t mask);

#endif // UTILITY_H
