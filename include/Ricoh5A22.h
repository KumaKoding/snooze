#ifndef RICOH_5A22_H
#define RICOH_5A22_H

#include <stdint.h>
#include "memory.h"

#define BUS_B_ACCESS_SPEED 3.58
#define INTERNAL_ACCESS_SPEED 3.58
#define BUS_A_ACCESS_SPEED 3.58
#define BUS_A_GENERAL_DMA_ACCESS_SPEED 2.68
#define CONTROLLER_PORT_ACCESS_SPEED 1.79

#define LE_HBYTE16(u16) (uint8_t)((u16 & 0xFF00) >> 8)
#define LE_LBYTE16(u16) (uint8_t)(u16 & 0x00FF)

#define SWP_LE_LBYTE16(u16, u8) ((u16 & 0xFF00) | u8)
#define SWP_LE_HBYTE16(u16, u8) ((u16 & 0x00FF) | ((0x0000 | u8) << 8))

#define LE_COMBINE_BANK_SHORT(bank, us) (uint32_t)(((((0x00000000 | bank) << 8) | ((us & 0xFF00) >> 8)) << 8) | (us & 0x00FF))
#define LE_COMBINE_BANK_2BYTE(bank, b1, b2) (uint32_t)(((((0x00000000 | bank) << 8) | b2) << 8) | b1)
#define LE_COMBINE_2BYTE(b1, b2) (uint16_t)(((0x0000 | b2) << 8) | b1)
#define LE_COMBINE_3BYTE(b1, b2, b3) (uint32_t)(((((0x00000000 | b3) << 8) | b2) << 8) | b1)

#define COP_VECTOR_65816 (uint32_t[]){ 0x0000FFE4, 0x0000FFE5 }
#define BRK_VECTOR_65816 (uint32_t[]){ 0x0000FFE6, 0x0000FFE7 }
#define ABORT_VECTOR_65816 (uint32_t[]){ 0x0000FFE8, 0x0000FFE9 }
#define NMI_VECTOR_65816 (uint32_t[]){ 0x0000FFEA, 0x0000FFEB }
#define IRQ_VECTOR_65816 (uint32_t[]){ 0x0000FFEE, 0x0000FFEF }

#define COP_VECTOR_6502 (uint32_t[]){ 0x0000FFF4, 0x0000FFF5 }
#define ABORT_VECTOR_6502 (uint32_t[]){ 0x0000FFF8, 0x0000FFF9 }
#define NMI_VECTOR_6502 (uint32_t[]){ 0x0000FFFA, 0x0000FFFB }
#define RESET_VECTOR_6502 (uint32_t[]){ 0x0000FFFC, 0x0000FFFD }
#define IRQ_VECTOR_6502 (uint32_t[]){ 0x0000FFFE, 0x0000FFFF }

#define BIT_SECL(bits, mask, cond) \
	if(cond) \
	{ \
		bits |= mask; \
	} \
	else \
	{ \
		bits &= ~mask; \
	}

#define CPU_STATUS_N 0x80 // Negative
#define CPU_STATUS_V 0x40 // Overflow
#define CPU_STATUS_M 0x20 // A register size (0 -> 16-bit, 1 -> 8-bit)
#define CPU_STATUS_X 0x10 // X, Y register size (0 -> 16-bit, 1 -> 8bit)
#define CPU_STATUS_D 0x08 // Decimal
#define CPU_STATUS_I 0x04 // Interrupt Request (IRQ) disable
#define CPU_STATUS_Z 0x02 // Zero
#define CPU_STATUS_C 0x01 // Carry
#define CPU_STATUS_E 0x01 // 6502 emulation mode (8-bit) -> set first bit to 0
#define CPU_STATUS_B 0x10 // Break (emulation mode)

/* SOURCE: https://www.westerndesigncenter.com/wdc/documentation/w65c816s.pdf
 *
 * OPCODE -suffix initialisms:
 * ABS     => Absolute, a
 * ABS_II  => Absolute Indexed Indirect, (a,x)
 * ABS_IIX => Absolute Indexed with X, a,x
 * ABS_IIY => Absolute Indexed with Y, a,y
 * ABS_I   => Absolute Indirect, (a)
 * ABS_LIX => Absolute Long Indexed with X, al,x
 * ABS_L   => Absolute Long, al
 * ACC     => Accumulator, A
 * XYC     => Block Move, xyc
 * DIR_IIX => Direct Indexed Indirect, (d,x)
 * DIR_IX  => Direct Indexed with X, d,x
 * DIR_IY  => Direct Indexed with Y, d,y
 * DIR_IIY => Direct Indexed Indirect, (d),y
 * DIR_ILI => Direct Indirect Long Indexed, [d],y
 * DIR_IL  => Direct Indirect Long, [d]
 * DIR_I   => Direct Indirect, (d)
 * DIR     => Direct, d
 * IMM     => Immediate, #
 * IMP     => Implied, i
 * REL_L   => Program Counter Relative Long, rl
 * REL     => Program Counter Relative, r
 * STK     => Stack, s
 * STK_R   => Stack Relative, d,s
 * STK_RII => Stack Relative Indirect Indexed, (d,s),y
*/

#define OPCODE_ADC_ABS 0x6D
#define OPCODE_ADC_ABS_IIX 0x7D
#define OPCODE_ADC_ABS_IIY 0x79
#define OPCODE_ADC_ABS_L 0x6F
#define OPCODE_ADC_ABS_LIX 0x7F
#define OPCODE_ADC_DIR 0x65
#define OPCODE_ADC_STK_R 0x63
#define OPCODE_ADC_DIR_IX 0x75
#define OPCODE_ADC_DIR_I 0x72
#define OPCODE_ADC_DIR_IL 0x67
#define OPCODE_ADC_STK_RII 0x73
#define OPCODE_ADC_DIR_IIX 0x61
#define OPCODE_ADC_DIR_IIY 0x71
#define OPCODE_ADC_DIR_ILI 0x77
#define OPCODE_ADC_IMM 0x69

#define OPCODE_AND_ABS 0x2D
#define OPCODE_AND_ABS_IIX 0x3D
#define OPCODE_AND_ABS_IIY 0x39
#define OPCODE_AND_ABS_L 0x2F
#define OPCODE_AND_ABS_LIX 0x3F
#define OPCODE_AND_DIR 0x25
#define OPCODE_AND_STK_R 0x23
#define OPCODE_AND_DIR_IX 0x35
#define OPCODE_AND_DIR_I 0x32
#define OPCODE_AND_DIR_IL 0x27
#define OPCODE_AND_STK_RII 0x33
#define OPCODE_AND_DIR_IIX 0x21
#define OPCODE_AND_DIR_IIY 0x31
#define OPCODE_AND_DIR_ILI 0x37
#define OPCODE_AND_IMM 0x29

#define OPCODE_ASL_ABS 0x0E
#define OPCODE_ASL_ACC 0x0A
#define OPCODE_ASL_ABS_IIX 0x1E
#define OPCODE_ASL_DIR 0x06
#define OPCODE_ASL_DIR_IX 0x16

#define OPCODE_BCC_REL 0x90

#define OPCODE_BCS_REL 0xB0

#define OPCODE_BEQ_REL 0xF0

#define OPCODE_BIT_ABS 0x2C
#define OPCODE_BIT_ABS_IIX 0x3C
#define OPCODE_BIT_DIR 0x24
#define OPCODE_BIT_DIR_IX 0x34
#define OPCODE_BIT_IMM 0x89

#define OPCODE_BMI_REL 0x30

#define OPCODE_BNE_REL 0xD0

#define OPCODE_BPL_REL 0x10

#define OPCODE_BRA_REL 0x80

#define OPCODE_BRK_STK 0x00

#define OPCODE_BRL_REL_L 0x82

#define OPCODE_BVC_REL 0x50

#define OPCODE_BVS_REL 0x70

#define OPCODE_CLC_IMP 0x18

#define OPCODE_CLD_IMP 0xD8

#define OPCODE_CLI_IMP 0x58

#define OPCODE_CLV_IMP 0xB8

#define OPCODE_CMP_ABS 0xCD
#define OPCODE_CMP_ABS_IIX 0xDD
#define OPCODE_CMP_ABS_IIY 0xD9
#define OPCODE_CMP_ABS_L 0xCF
#define OPCODE_CMP_ABS_LIX 0xDF
#define OPCODE_CMP_DIR 0xC5
#define OPCODE_CMP_STK_R 0xC3
#define OPCODE_CMP_DIR_IX 0xD5
#define OPCODE_CMP_DIR_I 0xD2
#define OPCODE_CMP_DIR_IL 0xC7
#define OPCODE_CMP_STK_RII 0xD3
#define OPCODE_CMP_DIR_IIX 0xC1
#define OPCODE_CMP_DIR_IIY 0xD1
#define OPCODE_CMP_DIR_ILI 0xD7
#define OPCODE_CMP_IMM 0xC9

#define OPCODE_COP_STK 0x02

#define OPCODE_CPX_ABS 0xEC
#define OPCODE_CPX_DIR 0xE4
#define OPCODE_CPX_IMM 0xE0

#define OPCODE_CPY_ABS 0xCC
#define OPCODE_CPY_DIR 0xC4
#define OPCODE_CPY_IMM 0xC0

#define OPCODE_DEC_ABS 0xCE
#define OPCODE_DEC_ACC 0x3A
#define OPCODE_DEC_ABS_IIX 0xDE
#define OPCODE_DEC_DIR 0xC6
#define OPCODE_DEC_DIR_IX 0xD6

#define OPCODE_DEX_IMP 0xCA

#define OPCODE_DEY_IMP 0x88

#define OPCODE_EOR_ABS 0x4D
#define OPCODE_EOR_ABS_IIX 0x5D
#define OPCODE_EOR_ABS_IIY 0x59
#define OPCODE_EOR_ABS_L 0x4F
#define OPCODE_EOR_ABS_LIX 0x5F
// #define OPCODE_EOR_ABS_II 0x5D
#define OPCODE_EOR_DIR 0x45
#define OPCODE_EOR_STK_R 0x43
#define OPCODE_EOR_DIR_IX 0x55
#define OPCODE_EOR_DIR_I 0x52
#define OPCODE_EOR_DIR_IL 0x47
#define OPCODE_EOR_STK_RII 0x53
#define OPCODE_EOR_DIR_IIX 0x41
#define OPCODE_EOR_DIR_IIY 0x51
#define OPCODE_EOR_DIR_ILI 0x57
#define OPCODE_EOR_IMM 0x49

#define OPCODE_INC_ABS 0xEE
#define OPCODE_INC_ACC 0x1A
#define OPCODE_INC_ABS_IIX 0xFE
#define OPCODE_INC_DIR 0xE6
#define OPCODE_INC_DIR_IX 0xF6

#define OPCODE_INX_IMP 0xE8

#define OPCODE_INY_IMP 0xC8

#define OPCODE_JML_ABS_IL 0xDC
#define OPCODE_JML_ABS_L 0x5C

#define OPCODE_JMP_ABS 0x4C
#define OPCODE_JMP_ABS_I 0x6C
#define OPCODE_JMP_ABS_II 0x7C

#define OPCODE_JSL_ABS_L 0x22

#define OPCODE_JSR_ABS 0x20
#define OPCODE_JSR_ABS_II 0xFC

#define OPCODE_LDA_ABS 0xAD
#define OPCODE_LDA_ABS_IIX 0xBD
#define OPCODE_LDA_ABS_IIY 0xB9
#define OPCODE_LDA_ABS_L 0xAF
#define OPCODE_LDA_ABS_LIX 0xBF
#define OPCODE_LDA_DIR 0xA5
#define OPCODE_LDA_STK_R 0xA3
#define OPCODE_LDA_DIR_IX 0xB5
#define OPCODE_LDA_DIR_I 0xB2
#define OPCODE_LDA_DIR_IL 0xA7
#define OPCODE_LDA_STK_RII 0xB3
#define OPCODE_LDA_DIR_IIX 0xA1
#define OPCODE_LDA_DIR_IIY 0xB1
#define OPCODE_LDA_DIR_ILI 0xB7
#define OPCODE_LDA_IMM 0xA9

#define OPCODE_LDX_ABS 0xAE
#define OPCODE_LDX_ABS_IIY 0xBE
#define OPCODE_LDX_DIR 0xA6
#define OPCODE_LDX_DIR_IY 0xB6
#define OPCODE_LDX_IMM 0xA2

#define OPCODE_LDY_ABS 0xAC
#define OPCODE_LDY_ABS_IIX 0xBC
#define OPCODE_LDY_DIR 0xA4
#define OPCODE_LDY_DIR_IX 0xB4
#define OPCODE_LDY_IMM 0xA0

#define OPCODE_LSR_ABS 0x4E
#define OPCODE_LSR_ACC 0x4A
#define OPCODE_LSR_ABS_IIX 0x5E
#define OPCODE_LSR_DIR 0x46
#define OPCODE_LSR_DIR_IX 0x56

#define OPCODE_MVN_XYC 0x54

#define OPCODE_MVP_XYC 0x44

#define OPCODE_NOP_IMP 0xEA

#define OPCODE_ORA_ABS 0x0D
#define OPCODE_ORA_ABS_IIX 0x1D
#define OPCODE_ORA_ABS_IIY 0x19
#define OPCODE_ORA_ABS_L 0x0F
#define OPCODE_ORA_ABS_LIX 0x1F
#define OPCODE_ORA_DIR 0x05
#define OPCODE_ORA_STK_R 0x03
#define OPCODE_ORA_DIR_IX 0x15
#define OPCODE_ORA_DIR_I 0x12
#define OPCODE_ORA_DIR_IL 0x07
#define OPCODE_ORA_STK_RII 0x13
#define OPCODE_ORA_DIR_IIX 0x01
#define OPCODE_ORA_DIR_IIY 0x11
#define OPCODE_ORA_DIR_ILI 0x17
#define OPCODE_ORA_IMM 0x09

#define OPCODE_PEA_STK 0xF4

#define OPCODE_PEI_STK 0xD4

#define OPCODE_PER_STK 0x62

#define OPCODE_PHA_STK 0x48

#define OPCODE_PHB_STK 0x8B

#define OPCODE_PHD_STK 0x0B

#define OPCODE_PHK_STK 0x4B

#define OPCODE_PHP_STK 0x08

#define OPCODE_PHX_STK 0xDA

#define OPCODE_PHY_STK 0x5A

#define OPCODE_PLA_STK 0x68

#define OPCODE_PLB_STK 0xAB

#define OPCODE_PLD_STK 0x2B

#define OPCODE_PLP_STK 0x28

#define OPCODE_PLX_STK 0xFA

#define OPCODE_PLY_STK 0x7A

#define OPCODE_REP_IMM 0xC2

#define OPCODE_ROL_ABS 0x2E
#define OPCODE_ROL_ACC 0x2A
#define OPCODE_ROL_ABS_IIX 0x3E
#define OPCODE_ROL_DIR 0x26
#define OPCODE_ROL_DIR_IX 0x36

#define OPCODE_ROR_ABS 0x6E
#define OPCODE_ROR_ACC 0x6A
#define OPCODE_ROR_ABS_IIX 0x7E
#define OPCODE_ROR_DIR 0x66
#define OPCODE_ROR_DIR_IX 0x76

#define OPCODE_RTI_STK 0x40

#define OPCODE_RTL_STK 0x6B

#define OPCODE_RTS_STK 0x60

#define OPCODE_SBC_ABS 0xED
#define OPCODE_SBC_ABS_IIX 0xFD
#define OPCODE_SBC_ABS_IIY 0xF9
#define OPCODE_SBC_ABS_LIX 0xFF
#define OPCODE_SBC_DIR 0xE5
#define OPCODE_SBC_STK_R 0xE3
#define OPCODE_SBC_DIR_IX 0xF5
#define OPCODE_SBC_DIR_I 0xF2
#define OPCODE_SBC_DIR_IL 0xE7
#define OPCODE_SBC_STK_RII 0xF3
#define OPCODE_SBC_DIR_IIX 0xE1
#define OPCODE_SBC_DIR_IIY 0xF1
#define OPCODE_SBC_DIR_ILI 0xF7
#define OPCODE_SBC_IMM 0xE9

#define OPCODE_SEC_IMP 0x38

#define OPCODE_SED_IMP 0xF8

#define OPCODE_SEI_IMP 0x78

#define OPCODE_SEP_IMM 0xE2

#define OPCODE_STA_ABS 0x8D
#define OPCODE_STA_ABS_IIX 0x9D
#define OPCODE_STA_ABS_L 0x8F
#define OPCODE_STA_ABS_LIX 0x9F
#define OPCODE_STA_DIR 0x85
#define OPCODE_STA_STK_R 0x83
#define OPCODE_STA_DIR_IX 0x95
#define OPCODE_STA_DIR_I 0x92
#define OPCODE_STA_DIR_IL 0x87
#define OPCODE_STA_STK_RII 0x93
#define OPCODE_STA_DIR_IIX 0x81
#define OPCODE_STA_DIR_IIY 0x91
#define OPCODE_STA_DIR_ILI 0x97

#define OPCODE_STP_IMP 0xDB

#define OPCODE_STX_ABS 0x8E
#define OPCODE_STX_DIR 0x86
#define OPCODE_STX_DIR_IY 0x96

#define OPCODE_STY_ABS 0x8C
#define OPCODE_STY_DIR 0x84
#define OPCODE_STY_DIR_IX 0x94

#define OPCODE_STZ_ABS 0x9C
#define OPCODE_STZ_ABS_IIX 0x9E
#define OPCODE_STZ_DIR 0x64
#define OPCODE_STZ_DIR_IX 0x74

#define OPCODE_TAX_IMP 0xAA

#define OPCODE_TAY_IMP 0xA8

#define OPCODE_TCD_IMP 0x5B

#define OPCODE_TCS_IMP 0x1B

#define OPCODE_TDC_IMP 0x7B

#define OPCODE_TRB_ABS 0x1C
#define OPCODE_TRB_DIR 0x14

#define OPCODE_TSB_ABS 0x0C
#define OPCODE_TSB_DIR 0x04

#define OPCODE_TSC_IMP 0x3B

#define OPCODE_TSX_IMP 0xBA

#define OPCODE_TXA_IMP 0x8A

#define OPCODE_TXS_IMP 0x9A

#define OPCODE_TXY_IMP 0x9B

#define OPCODE_TYA_IMP 0x98

#define OPCODE_TYX_IMP 0xBB

#define OPCODE_WAI_IMP 0xCB

#define OPCODE_WDM_IMM 0x42

#define OPCODE_XBA_IMP 0xEB

#define OPCODE_XCE_IMP 0xFB

struct Ricoh_5A22
{
	uint16_t register_X;
	uint16_t register_Y;

	uint16_t register_A;

	uint16_t stack_ptr;

	uint16_t direct_page;

	uint8_t data_bank;

	uint8_t program_bank;
	uint16_t program_ctr;

	uint8_t cpu_emulation6502;
	uint8_t cpu_status;

	int LPM;
	int RDY;

	int NMI_line;
	int IRQ_line;
};

void reset_ricoh_5a22(struct Ricoh_5A22 *cpu, struct Memory *memory);
void decode_execute(struct Ricoh_5A22 *cpu, struct Memory *memory);

#endif // RICOH_5A22_H

