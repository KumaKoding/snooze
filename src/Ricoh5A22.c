#include "Ricoh5A22.h"
#include "memory.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef int bool;

#define true 1
#define false 0

uint8_t check_bit8(uint8_t ps, uint8_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

uint8_t check_bit16(uint16_t ps, uint16_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

uint8_t check_bit32(uint32_t ps, uint32_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

void swap_cpu_status(struct Ricoh_5A22 *cpu, uint8_t new_flags)
{
	cpu->cpu_status = new_flags;

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		cpu->cpu_status |= CPU_STATUS_M;
		cpu->cpu_status |= CPU_STATUS_X;
	}

	if(check_bit8(cpu->cpu_status, CPU_STATUS_X))
	{
		cpu->register_X &= 0x00FF;
		cpu->register_Y &= 0x00FF;
	}
}

int accumulator_size(struct Ricoh_5A22 *cpu)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) || check_bit8(cpu->cpu_status, CPU_STATUS_M))
	{
		return 8;
	}
	else 
	{
		return 16;
	}
}

int index_size(struct Ricoh_5A22 *cpu)
{

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) || check_bit8(cpu->cpu_status, CPU_STATUS_X))
	{
		return 8;
	}
	else 
	{
		return 16;
	}
}


#define get_A(cpu) ((accumulator_size(cpu) == 8) ? LE_LBYTE16(cpu->register_A) : cpu->register_A)
#define get_X(cpu) ((index_size(cpu) == 8) ? LE_LBYTE16(cpu->register_X) : cpu->register_X)
#define get_Y(cpu) ((index_size(cpu) == 8) ? LE_LBYTE16(cpu->register_Y) : cpu->register_Y)
#define get_SP(cpu) ((index_size(cpu) == 8) ? SWP_LE_HBYTE16(cpu->stack_ptr, 0x01) : cpu->stack_ptr)

void decrement_SP(struct Ricoh_5A22 *cpu, int n)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		cpu->stack_ptr = SWP_LE_LBYTE16(cpu->stack_ptr, LE_LBYTE16(cpu->stack_ptr) - n);
	}
	else 
	{
		cpu->stack_ptr -= n;
	}
}

void increment_SP(struct Ricoh_5A22 *cpu, int n)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		cpu->stack_ptr = SWP_LE_LBYTE16(cpu->stack_ptr, LE_LBYTE16(cpu->stack_ptr) + n);
	}
	else 
	{
		cpu->stack_ptr += n;
	}
}

void push_SP(struct Ricoh_5A22 *cpu, struct Memory *memory, uint8_t write_val)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		DB_write(memory, get_SP(cpu), write_val);
		decrement_SP(cpu, 1);
	}
	else 
	{
		DB_write(memory, get_SP(cpu), write_val);
		decrement_SP(cpu, 1);
	}
}

uint8_t pull_SP(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		increment_SP(cpu, 1);
		return DB_read(memory, get_SP(cpu));
	}
	else 
	{
		increment_SP(cpu, 1);
		return DB_read(memory, get_SP(cpu));
	}
}

uint32_t addr_ABS(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, oper), DB_read(memory, oper + 1));

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_IIX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, oper), DB_read(memory, oper + 1)) + get_X(cpu);

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_IIY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, oper), DB_read(memory, oper + 1)) + get_Y(cpu);

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_L(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_3BYTE(DB_read(memory, oper), DB_read(memory, oper + 1), DB_read(memory, oper + 2));

	cpu->program_ctr += 3;

	return addr;
}

uint32_t addr_ABS_LIX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = LE_COMBINE_3BYTE(DB_read(memory, oper), DB_read(memory, oper + 1), DB_read(memory, oper + 2)) + get_X(cpu);

	cpu->program_ctr += 3;

	return addr;
}

uint32_t addr_ABS_I(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_2BYTE(DB_read(memory, oper), DB_read(memory, oper + 1));
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->program_bank, addr_indirect);

	cpu->program_ctr += 2;

	return addr_effective;
}

uint32_t addr_ABS_IL(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_2BYTE(DB_read(memory, oper), DB_read(memory, oper + 1));
	uint32_t addr_indirect = LE_COMBINE_3BYTE(DB_read(memory, addr), DB_read(memory, addr + 1), DB_read(memory, addr + 2));

	cpu->program_ctr += 2;

	return addr_indirect;
}

uint32_t addr_ABS_II(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = LE_COMBINE_2BYTE(DB_read(memory, oper), DB_read(memory, oper + 1)) + get_X(cpu);
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->program_bank, addr_indirect);

	cpu->program_ctr += 2;

	return addr_effective;
}

uint32_t addr_DIR(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_STK_R(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = get_SP(cpu) + DB_read(memory, oper);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper) + get_X(cpu);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper) + get_Y(cpu);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_I(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->data_bank, addr_indirect);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IL(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_effective = LE_COMBINE_3BYTE(DB_read(memory, addr), DB_read(memory, addr + 1), DB_read(memory, addr + 2));

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_STK_RII(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = get_SP(cpu) + DB_read(memory, oper);
	uint32_t addr_base = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = addr_base + get_Y(cpu);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IIX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper) + get_X(cpu);
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->data_bank, addr_indirect);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IIY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_indirect = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = addr_indirect + get_Y(cpu);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_ILI(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_indirect = LE_COMBINE_3BYTE(DB_read(memory, addr), DB_read(memory, addr + 1), DB_read(memory, addr + 2));
	uint32_t addr_effective = addr_indirect + get_Y(cpu);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_REL(struct Ricoh_5A22 *cpu)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 1;

	return oper;
}

uint32_t addr_REL_L(struct Ricoh_5A22 *cpu)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 2;

	return oper;
}

uint32_t addr_IMM_8(struct Ricoh_5A22 *cpu)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 1;

	return oper;
}

uint32_t addr_IMM_16(struct Ricoh_5A22 *cpu)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 2;

	return oper;
}

uint32_t addr_IMM_M(struct Ricoh_5A22 *cpu)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	if(accumulator_size(cpu) == 8)
	{
		cpu->program_ctr += 1;
	}
	else 
	{
		cpu->program_ctr += 2;
	}

	return oper;
}

uint32_t addr_IMM_X(struct Ricoh_5A22 *cpu)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	if(index_size(cpu) == 8)
	{
		cpu->program_ctr += 1;
	}
	else 
	{
		cpu->program_ctr += 2;
	}

	return oper;
}

uint32_t addr_XYC(struct Ricoh_5A22 *cpu)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 2;

	return oper;
}

void ADC(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	bool carry = false;
	bool negative = false;
	bool overflow = false;
	bool zero = false;

	if(accumulator_size(cpu) == 8) // 8 bit
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint8_t cin = check_bit8(cpu->cpu_status, CPU_STATUS_C);

			uint8_t opr_digit_0 = operand & 0x0F;
			uint8_t acc_digit_0 = accumulator & 0x0F;

			uint8_t opr_digit_1 = (operand & 0xF0) >> 4;
			uint8_t acc_digit_1 = (accumulator & 0xF0) >> 4;

			uint8_t suml_0 = acc_digit_0 ^ opr_digit_0;
			uint8_t coutl_0 = acc_digit_0 & opr_digit_0;

			uint8_t suml_1 = suml_0 ^ cin;
			uint8_t coutl_1 = (suml_0 & cin) | coutl_0;

			coutl_1 = coutl_1 << 1;

			while(coutl_1 != 0)
			{
				suml_0 = suml_1;
				coutl_0 = coutl_1;

				suml_1 = suml_0 ^ coutl_0;
				coutl_1 = suml_0 & coutl_0;

				coutl_1 = coutl_1 << 1;
			}


			if(suml_1 > 0x09)
			{
				suml_1 += 0x06;
				cin = 0x01;
			}
			else 
			{
				cin = 0x00;
			}

			uint8_t sumh_0 = acc_digit_1 ^ opr_digit_1;
			uint8_t couth_0 = acc_digit_1 & opr_digit_1;

			uint8_t sumh_1 = sumh_0 ^ cin;
			uint8_t couth_1 = (sumh_0 & cin) | couth_0;

			if(check_bit8(couth_1, 0x40))
			{
				overflow = true;
			}

			couth_1 = couth_1 << 1;

			while(couth_1 != 0)
			{
				sumh_0 = sumh_1;
				couth_0 = couth_1;

				sumh_1 = sumh_0 ^ couth_0;
				couth_1 = sumh_0 & couth_0;

				if(check_bit8(couth_1, 0x40))
				{
					overflow = true;
				}

				couth_1 = couth_1 << 1;
			}


			if(sumh_1 > 0x09)
			{
				sumh_1 += 0x06;

				carry = true;
			}
			else 
			{
				cin = 0x00;
			}

			if(check_bit8(sumh_1, 0x80))
			{
				negative = true;	
			}

			if(((sumh_1 << 4) | suml_1) == 0)
			{
				zero = true;
			}

			suml_1 &= 0x0F;
			sumh_1 &= 0x0F;

			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, (sumh_1 << 4) | suml_1);
		}
		else 
		{
			uint8_t cin = check_bit8(cpu->cpu_status, CPU_STATUS_C);

			uint8_t sum_0 = accumulator ^ operand;
			uint8_t cout_0 = accumulator & operand;

			uint8_t sum_1 = sum_0 ^ cin;
			uint8_t cout_1 = (sum_0 & cin) | cout_0;

			if(check_bit8(cout_1, 0x80))
			{
				carry = true;
			}

			if(check_bit8(cout_1, 0x40))
			{
				overflow = true;
			}

			cout_1 = cout_1 << 1;

			while(cout_1 != 0)
			{
				sum_0 = sum_1;
				cout_0 = cout_1;

				sum_1 = sum_0 ^ cout_0;
				cout_1 = sum_0 & cout_0;

				if(check_bit8(cout_1, 0x80))
				{
					carry = true;
				}

				if(check_bit8(cout_1, 0x40))
				{
					overflow = true;
				}

				cout_1 = cout_1 << 1;
			}

			if(check_bit8(cout_1, 0x80))
			{
				negative = true;
			}

			if(sum_1 == 0)
			{
				zero = true;
			}


			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, sum_1);
		}
	}
	else  // 16 bit
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint16_t cin = (uint16_t)check_bit8(cpu->cpu_status, CPU_STATUS_C);

			uint8_t opr_digit_0 = operand & 0x000F;
			uint8_t acc_digit_0 = accumulator & 0x000F;

			uint8_t opr_digit_1 = (operand & 0x00F0) >> 4;
			uint8_t acc_digit_1 = (accumulator & 0x00F0) >> 4;

			uint8_t opr_digit_2 = (operand & 0x0F00) >> 8;
			uint8_t acc_digit_2 = (accumulator & 0x0F00) >> 8;

			uint8_t opr_digit_3 = (operand & 0xF000) >> 12;
			uint8_t acc_digit_3 = (accumulator & 0xF000) >> 12;

			uint8_t sumal_0 = acc_digit_0 ^ opr_digit_0;
			uint8_t coutal_0 = acc_digit_0 & opr_digit_0;

			uint8_t sumal_1 = sumal_0 ^ cin;
			uint8_t coutal_1 = (sumal_0 & cin) | coutal_0;

			coutal_1 = coutal_1 << 1;

			while(coutal_1 != 0)
			{
				sumal_0 = sumal_1;
				coutal_0 = coutal_1;

				sumal_1 = sumal_0 ^ coutal_0;
				coutal_1 = sumal_0 & coutal_0;
	
				coutal_1 = coutal_1 << 1;
			}

			if(sumal_1 > 0x09)
			{
				sumal_1 += 0x06;
				cin = 0x01;
			}
			else 
			{
				cin = 0x00;
			}

			uint8_t sumah_0 = acc_digit_1 ^ opr_digit_1;
			uint8_t coutah_0 = acc_digit_1 & opr_digit_1;

			uint8_t sumah_1 = sumah_0 ^ cin;
			uint8_t coutah_1 = (sumah_0 & cin) | coutah_0;

			coutah_1 = coutah_1 << 1;

			while(coutah_1 != 0)
			{
				sumah_0 = sumah_1;
				coutah_0 = coutah_1;

				sumah_1 = sumah_0 ^ coutah_0;
				coutah_1= sumah_0 & coutah_0;

				coutah_1 = coutah_1 << 1;
			}

			if(sumah_1 > 0x09)
			{
				sumah_1 += 0x06;
				cin = 0x01;
			}
			else 
			{
				cin = 0x00;
			}

			uint8_t sumbl_0 = acc_digit_2 ^ opr_digit_2;
			uint8_t coutbl_0 = acc_digit_2 & opr_digit_2;

			uint8_t sumbl_1 = sumbl_0 ^ cin;
			uint8_t coutbl_1 = (sumbl_0 & cin) | coutbl_0;

			coutbl_1 = coutbl_1 << 1;

			while(coutbl_1 != 0)
			{
				sumbl_0 = sumbl_1;
				coutbl_0 = coutbl_1;

				sumbl_1 = sumbl_0 ^ coutbl_0;
				coutbl_1 = sumbl_0 & coutbl_0;

				coutbl_1 = coutbl_1 << 1;
			}

			if(sumbl_1 > 0x09)
			{
				sumbl_1 += 0x06;
				cin = 0x01;
			}
			else 
			{
				cin = 0x00;
			}

			uint8_t sumbh_0 = acc_digit_3 ^ opr_digit_3;
			uint8_t coutbh_0 = acc_digit_3 & opr_digit_3;

			uint8_t sumbh_1 = sumbh_0 ^ cin;
			uint8_t coutbh_1 = (sumbh_0 & cin) | coutbh_0;

			if(check_bit8(coutbh_1, 0x40))
			{
				overflow = true;
			}

			coutbh_1 = coutbh_1 << 1;

			while(coutbh_1 != 0)
			{
				sumbh_0 = sumbh_1;
				coutbh_0 = coutbh_1;

				sumbh_1 = sumbh_0 ^ coutbh_0;
				coutbh_1 = sumbh_0 & coutbh_0;

				if(check_bit8(coutbh_1, 0x40))
				{
					overflow = true;
				}

				coutbh_1 = coutbh_1 << 1;
			}

			if(sumbh_1 > 0x09)
			{
				sumbh_1 += 0x06;

				carry = true;
			}
			else 
			{
				cin = 0x00;
			}

			if(check_bit8(sumbh_1, 0x80))
			{
				negative = true;	
			}

			if((((((((0x0000 | sumbh_1) << 4) | sumbl_1) << 4) | sumah_1) << 4) | sumal_1) == 0)
			{
				zero = true;
			}

			sumal_1 &= 0x0F;
			sumah_1 &= 0x0F;
			sumbl_1 &= 0x0F;
			sumbh_1 &= 0x0F;

			printf("%02x %02x %02x %02x\n", sumbh_1, sumbl_1, sumah_1, sumal_1);

			cpu->register_A = ((((((0x0000 | sumbh_1) << 4) | sumbl_1) << 4) | sumah_1) << 4) | sumal_1;
		}
		else 
		{
			uint16_t cin = (uint16_t)check_bit8(cpu->cpu_status, CPU_STATUS_C);

			uint16_t sum_0 = accumulator ^ operand;
			uint16_t cout_0 = accumulator & operand;

			uint16_t sum_1 = sum_0 ^ cin;
			uint16_t cout_1 = (sum_0 & cin) | cout_0;

			if(check_bit16(cout_1, 0x8000))
			{
				carry = true;
			}

			if(check_bit16(cout_1, 0x4000))
			{
				overflow = true;
			}

			cout_1 = cout_1 << 1;

			while(cout_1 != 0)
			{
				sum_0 = sum_1;
				cout_0 = cout_1;

				sum_1 = sum_0 ^ cout_0;
				cout_1 = sum_0 & cout_0;

				if(check_bit16(cout_1, 0x8000))
				{
					carry = true;
				}

				if(check_bit16(cout_1, 0x4000))
				{
					carry = true;
				}

				cout_1 = cout_1 << 1;
			}

			if(check_bit16(cout_1, 0x8000))
			{
				negative = true;
			}

			if(sum_1 == 0)
			{
				zero = true;
			}

			cpu->register_A = sum_1;
		}
	}

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, negative);
	BIT_SECL(cpu->cpu_status, CPU_STATUS_V, overflow);
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, zero);
	BIT_SECL(cpu->cpu_status, CPU_STATUS_C, carry);
}

void AND(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = result;
	}
}

void ASL(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x80));
		
		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		DB_write(memory, addr, operand);
		printf("adlkjfa\n");
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		DB_write(memory, addr, LE_LBYTE16(operand));
		DB_write(memory, addr + 1, LE_HBYTE16(operand));
	}
}

void ASL_A(struct Ricoh_5A22 *cpu)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x80));
		
		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, operand);
	}
	else 
	{
		uint16_t operand = get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = operand;
	}
}

void BCC(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_C))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void BCS(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_C))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void BEQ(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_Z))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void BIT(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit8(operand, 0x40));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit16(operand, 0x4000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
}

void BIT_IMM(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
}

void BMI(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_N))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void BNE(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_Z))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void BPL(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_N))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void BRA(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	cpu->program_ctr += (int8_t)operand;
}

void BRK(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		push_SP(cpu, memory, LE_HBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, LE_LBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_B;
		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(BRK_VECTOR_65816[0], BRK_VECTOR_65816[1]);
	}
	else 
	{
		push_SP(cpu, memory, cpu->program_bank);
		push_SP(cpu, memory, LE_HBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, LE_LBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(BRK_VECTOR_65816[0], BRK_VECTOR_65816[1]);
	}
}

void BRL(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

	cpu->program_ctr += operand;
}

void BVC(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_V))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void BVS(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_V))
	{
		cpu->program_ctr += (int8_t)operand;
	}
}

void CLC(struct Ricoh_5A22 *cpu)
{
	cpu->cpu_status &= ~CPU_STATUS_C;
}

void CLD(struct Ricoh_5A22 *cpu)
{
	cpu->cpu_status &= ~CPU_STATUS_D;
}

void CLI(struct Ricoh_5A22 *cpu)
{
	cpu->cpu_status &= ~CPU_STATUS_I;
}

void CLV(struct Ricoh_5A22 *cpu)
{
	cpu->cpu_status &= ~CPU_STATUS_V;
}

void CMP(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = accumulator - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = accumulator - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
}

void COP(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		push_SP(cpu, memory, LE_HBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, LE_LBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_B;
		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(COP_VECTOR_65816[0], COP_VECTOR_65816[1]);
	}
	else 
	{
		push_SP(cpu, memory, cpu->program_bank);
		push_SP(cpu, memory, LE_HBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, LE_LBYTE16(cpu->program_ctr));
		push_SP(cpu, memory, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(COP_VECTOR_65816[0], COP_VECTOR_65816[1]);
	}
}

void CPX(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t index = get_X(cpu);

		uint8_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t index = get_X(cpu);

		uint16_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
}

void CPY(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t index = get_Y(cpu);

		uint8_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t index = get_Y(cpu);

		uint16_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
}

void DEC(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		
		uint8_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

		uint16_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, LE_LBYTE16(result));
		DB_write(memory, addr + 1, LE_HBYTE16(result));
	}
}

void DEC_A(struct Ricoh_5A22 *cpu)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);
		
		uint8_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = get_A(cpu);

		uint16_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void DEX(struct Ricoh_5A22 *cpu)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = get_X(cpu);
		uint8_t result = index - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, result);
	}
	else 
	{
		uint16_t index = get_X(cpu);
		uint16_t result = index - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_X = result;
	}
}

void DEY(struct Ricoh_5A22 *cpu)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = get_Y(cpu);
		uint8_t result = index - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, result);
	}
	else 
	{
		uint16_t index = get_Y(cpu);
		uint16_t result = index - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_Y = result;
	}
}

void EOR(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = operand ^ accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = operand ^ accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void INC(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		
		uint8_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

		uint16_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, LE_LBYTE16(result));
		DB_write(memory, addr + 1, LE_HBYTE16(result));
	}
}

void INC_A(struct Ricoh_5A22 *cpu)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);
		
		uint8_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = get_A(cpu);

		uint16_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void INX(struct Ricoh_5A22 *cpu)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = get_X(cpu);
		uint8_t result = index + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, result);
	}
	else 
	{
		uint16_t index = get_X(cpu);
		uint16_t result = index + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_X = result;
	}
}

void INY(struct Ricoh_5A22 *cpu)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = get_Y(cpu);
		uint8_t result = index + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, result);
	}
	else 
	{
		uint16_t index = get_Y(cpu);
		uint16_t result = index + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_Y = result;
	}
}

void JMP(struct Ricoh_5A22 *cpu, uint32_t addr)
{
	cpu->program_bank = (addr & 0x00FF0000) >> 16;
	cpu->program_ctr = addr & 0x0000FFFF;
}

void JSR(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	push_SP(cpu, memory, LE_HBYTE16(cpu->program_bank - 1));
	push_SP(cpu, memory, LE_LBYTE16(cpu->program_bank - 1));

	cpu->program_ctr = addr & 0x0000FFFF;
}

void JSL(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	push_SP(cpu, memory, cpu->program_bank);
	push_SP(cpu, memory, LE_HBYTE16(cpu->program_bank - 1));
	push_SP(cpu, memory, LE_LBYTE16(cpu->program_bank - 1));

	cpu->program_bank = (addr & 0x00FF0000) >> 16;
	cpu->program_ctr = addr & 0x0000FFFF;
}

void LDA(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = operand;
	}
}

void LDX(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_X  = SWP_LE_LBYTE16(cpu->register_X, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_X = operand;
	}
}

void LDY(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_Y = operand;
	}
}

void LSR(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x01));

		uint8_t result = operand >> 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x0001));

		uint16_t result = operand >> 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, LE_LBYTE16(result));
		DB_write(memory, addr + 1, LE_HBYTE16(result));
	}
}

void LSR_A(struct Ricoh_5A22 *cpu)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x01));

		uint8_t result = operand >> 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A &= 0xFF00;
		cpu->register_A |= result;
	}
	else 
	{
		uint16_t operand = get_A(cpu);
		
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x0001));

		uint16_t result = operand >> 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void MVN(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t src_bank = DB_read(memory, addr);
	uint8_t dst_bank = DB_read(memory, addr + 1);

	cpu->data_bank = dst_bank;

	uint32_t src_addr = LE_COMBINE_BANK_SHORT(src_bank, get_X(cpu));
	uint32_t dst_addr = LE_COMBINE_BANK_SHORT(cpu->data_bank, get_Y(cpu));

	if(get_A(cpu) != 0xFFFF)
	{
		uint8_t read_byte = DB_read(memory, src_addr);
		DB_write(memory, dst_addr, read_byte);

		cpu->program_ctr -= 3;

		if(index_size(cpu) == 8)
		{
			cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, (uint8_t)get_X(cpu) + 1);
			cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, (uint8_t)get_Y(cpu) + 1);
		}
		else 
		{	
			cpu->register_X = get_X(cpu) + 1;
			cpu->register_Y = get_Y(cpu) + 1;
		}

		cpu->register_A--;
	}
}

void MVP(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t src_bank = DB_read(memory, addr);
	uint8_t dst_bank = DB_read(memory, addr + 1);

	cpu->data_bank = dst_bank;

	uint32_t src_addr = LE_COMBINE_BANK_SHORT(src_bank, get_X(cpu));
	uint32_t dst_addr = LE_COMBINE_BANK_SHORT(cpu->data_bank, get_Y(cpu));

	if(get_A(cpu) != 0xFFFF)
	{
		uint8_t read_byte = DB_read(memory, src_addr);
		DB_write(memory, dst_addr, read_byte);

		cpu->program_ctr -= 3;

		if(index_size(cpu) == 8)
		{
			cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, (uint8_t)get_X(cpu) - 1);
			cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, (uint8_t)get_Y(cpu) - 1);
		}
		else 
		{	
			cpu->register_X = get_X(cpu) - 1;
			cpu->register_Y = get_Y(cpu) - 1;
		}

		cpu->register_A--;
	}
}

void NOP()
{
	// one internal operation cycle
	
	return;
}

void ORA(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = operand | accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = operand | accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void PEA(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t addr_l = (uint8_t)(addr & 0x000000FF);
	uint8_t addr_h = (uint8_t)((addr & 0x0000FF00) >> 8);

	push_SP(cpu, memory, addr_h);
	push_SP(cpu, memory, addr_l);
}

void PEI(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t addr_l = (uint8_t)(addr & 0x000000FF);
	uint8_t addr_h = (uint8_t)((addr & 0x0000FF00) >> 8);

	push_SP(cpu, memory, addr_h);
	push_SP(cpu, memory, addr_l);
}

void PER(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

	// 
	// CHECK FOR OVERFLOWS
	//

	operand += cpu->program_ctr;

	push_SP(cpu, memory, LE_HBYTE16(operand));
	push_SP(cpu, memory, LE_LBYTE16(operand));
}

void PHA(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t accumulator = get_A(cpu);

		push_SP(cpu, memory, accumulator);
	}
	else 
	{
		uint16_t accumulator = get_A(cpu);
		uint8_t acc_l = LE_LBYTE16(accumulator);
		uint8_t acc_h = LE_HBYTE16(accumulator);

		push_SP(cpu, memory, acc_h);
		push_SP(cpu, memory, acc_l);
	}
}

void PHB(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	push_SP(cpu, memory, cpu->data_bank);
}

void PHD(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint8_t direct_l = LE_LBYTE16(cpu->direct_page);
	uint8_t direct_h = LE_HBYTE16(cpu->direct_page);

	push_SP(cpu, memory, direct_h);
	push_SP(cpu, memory, direct_l);
}

void PHK(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	push_SP(cpu, memory, cpu->program_bank);
}

void PHP(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	push_SP(cpu, memory, cpu->cpu_status);
}

void PHX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = get_X(cpu);

		push_SP(cpu, memory, index);
	}
	else 
	{
		uint16_t index = get_X(cpu);
		uint8_t index_l = LE_LBYTE16(index);
		uint8_t index_h = LE_HBYTE16(index);

		push_SP(cpu, memory, index_h);
		push_SP(cpu, memory, index_l);
	}
}

void PHY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = get_Y(cpu);

		push_SP(cpu, memory, index);
	}
	else 
	{
		uint16_t index = get_Y(cpu);
		uint8_t index_l = LE_LBYTE16(index);
		uint8_t index_h = LE_HBYTE16(index);

		push_SP(cpu, memory, index_h);
		push_SP(cpu, memory, index_l);
	}
}

void PLA(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t accumulator = pull_SP(cpu, memory);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(accumulator, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (accumulator == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, accumulator);
	}
	else 
	{
		uint8_t accumulator_l = pull_SP(cpu, memory);
		uint8_t accumulator_h = pull_SP(cpu, memory);

		uint16_t result = LE_COMBINE_2BYTE(accumulator_h, accumulator_l);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void PLB(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint8_t bank = pull_SP(cpu, memory);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(bank, 0x80));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (bank == 0));

	cpu->data_bank = bank;
}

void PLD(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint8_t direct_l = pull_SP(cpu, memory);
	uint8_t direct_h = pull_SP(cpu, memory);

	uint16_t result = LE_COMBINE_2BYTE(direct_l, direct_h);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x80));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

	cpu->direct_page = result;
}

void PLP(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint8_t cpu_status = pull_SP(cpu, memory);

	cpu->cpu_status = cpu_status;
}

void PLX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = pull_SP(cpu, memory);		

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(index, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (index == 0));

		cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, index);
	}
	else 
	{
		uint8_t index_l = pull_SP(cpu, memory);
		uint8_t index_h = pull_SP(cpu, memory);

		uint16_t result = LE_COMBINE_2BYTE(index_l, index_h);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_X = result;
	}
}

void PLY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(index_size(cpu) == 8)
	{
		uint8_t index = pull_SP(cpu, memory);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(index, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (index == 0));

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, index);
	}
	else 
	{
		uint8_t index_l = pull_SP(cpu, memory);
		uint8_t index_h = pull_SP(cpu, memory);
	
		uint16_t result = LE_COMBINE_2BYTE(index_l, index_h);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_Y = result;
	}
}

void REP(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);
	uint8_t result = cpu->cpu_status & (~operand);

	swap_cpu_status(cpu, result);
}

void ROL(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t result = operand << 1;

		BIT_SECL(result, 0x01, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x80));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t result = operand << 1;

		BIT_SECL(result, 0x0001, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, LE_LBYTE16(result));
		DB_write(memory, addr + 1, LE_HBYTE16(result));
	}
}

void ROL_A(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t accumulator = get_A(cpu);
		uint8_t result = accumulator << 1;

		BIT_SECL(result, 0x01, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(accumulator, 0x80));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t accumulator = cpu->register_A;
		uint16_t result = accumulator << 1;

		BIT_SECL(result, 0x0001, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(accumulator, 0x8000));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void ROR(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t result = operand >> 1;

		BIT_SECL(result, 0x80, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x01));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t result = operand >> 1;

		BIT_SECL(result, 0x8000, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x0001));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		DB_write(memory, addr, LE_LBYTE16(result));
		DB_write(memory, addr + 1, LE_HBYTE16(result));
	}
}

void ROR_A(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t accumulator = get_A(cpu);
		uint8_t result = accumulator >> 1;

		BIT_SECL(result, 0x80, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(accumulator, 0x01));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t accumulator = cpu->register_A;
		uint16_t result = accumulator >> 1;

		BIT_SECL(result, 0x8000, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(accumulator, 0x0001));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void RTI(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		swap_cpu_status(cpu, pull_SP(cpu, memory));

		uint8_t pc_l = pull_SP(cpu, memory);
		uint8_t pc_h = pull_SP(cpu, memory);

		cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);
		cpu->program_bank = pull_SP(cpu, memory);
	}
	else 
	{
		swap_cpu_status(cpu, pull_SP(cpu, memory));

		uint8_t pc_l = pull_SP(cpu, memory);
		uint8_t pc_h = pull_SP(cpu, memory);

		cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);
	}
}

void RTL(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint8_t pc_l = pull_SP(cpu, memory);
	uint8_t pc_h = pull_SP(cpu, memory);

	cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);
	cpu->program_bank = pull_SP(cpu, memory);

	cpu->program_ctr++;
}

void RTS(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint8_t pc_l = pull_SP(cpu, memory);
	uint8_t pc_h = pull_SP(cpu, memory);

	cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);

	cpu->program_ctr++;
}

void SBC(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint8_t operand = DB_read(memory, addr);
			uint32_t nines = 0x00000099 - operand;
			uint32_t tens = nines + 0x00000001;
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x000000FF;

			uint32_t difference = wide_acc + tens - check_bit8(cpu->cpu_status, CPU_STATUS_C);

			if((difference & 0x0F) > 0x09)
			{
				difference += 0x06;
			}

			if((difference & 0xF0) > 0x90)
			{
				difference += 0x60;
			}
			printf("%08x\n", difference);

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(difference, 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint8_t)difference ^ get_A(cpu), 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(difference, 0x00000100));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (difference == 0));

			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, difference & 0x000000FF);
		}
		else 
		{
			uint8_t operand = DB_read(memory, addr);
			uint32_t operand_c = ((uint32_t)(~operand) & 0x000000FF);
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x000000FF;

			uint32_t difference = wide_acc + (operand_c + 1) - check_bit8(cpu->cpu_status, CPU_STATUS_C);
			printf("%08x\n", difference);

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(difference, 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint8_t)difference ^ get_A(cpu), 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(difference, 0x00000100));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (difference == 0));

			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, difference & 0x000000FF);
		}
	}
	else 
	{
		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
			uint32_t nines = 0x00009999 - operand;
			uint32_t tens = nines + 0x00000001;
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x0000FFFF;
			printf("%08x\n", wide_acc);
			printf("%08x\n", operand);

			uint32_t difference = wide_acc + (uint32_t)tens - check_bit8(cpu->cpu_status, CPU_STATUS_C);
	
			if((difference & 0x000F) > 0x0009)
			{
				difference += 0x0006;
			}

			if((difference & 0x00F0) > 0x0090)
			{
				difference += 0x0060;
			}

			if((difference & 0x0F00) > 0x0900)
			{
				difference += 0x0600;
			}

			if((difference & 0xF000) > 0x9000)
			{
				difference += 0x6000;
			}

			printf("%08x\n", difference);
			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(difference, 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint16_t)difference ^ get_A(cpu), 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(difference, 0x00010000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (difference == 0));

			cpu->register_A = (uint16_t)(difference & 0x0000FFFF);
		}
		else 
		{
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
			uint32_t operand_c = ((uint32_t)(~operand) & 0x0000FFFF);
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x0000FFFF;

			uint32_t difference = wide_acc + (operand_c + 1) - check_bit8(cpu->cpu_status, CPU_STATUS_C);

			printf("%08x\n", difference);

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(difference, 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint8_t)difference ^ get_A(cpu), 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(difference, 0x00010000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (difference == 0));

			cpu->register_A = (uint16_t)(difference & 0x00FFFF);
		}
	}
}

void SEC(struct Ricoh_5A22 *cpu)
{
	cpu->cpu_status |= CPU_STATUS_C;
}

void SED(struct Ricoh_5A22 *cpu)
{
	cpu->cpu_status |= CPU_STATUS_D;
}

void SEI(struct Ricoh_5A22 *cpu)
{
	cpu->cpu_status |= CPU_STATUS_I;
}

void SEP(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint8_t operand = DB_read(memory, addr);

	swap_cpu_status(cpu, cpu->cpu_status | operand);
}

void STA(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		DB_write(memory, addr, LE_LBYTE16(cpu->register_A));
	}
	else 
	{
		DB_write(memory, addr, LE_LBYTE16(cpu->register_A));
		DB_write(memory, addr + 1, LE_HBYTE16(cpu->register_A));
	}
}

void STP(struct Ricoh_5A22 *cpu)
{
	// takes 2 cycles like normal, but the last cycle is to halt the program
	cpu->LPM = 1;
}

void STX(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(index_size(cpu) == 8)
	{
		DB_write(memory, addr, LE_LBYTE16(cpu->register_X));
	}
	else 
	{
		DB_write(memory, addr, LE_LBYTE16(cpu->register_X));
		DB_write(memory, addr + 1, LE_HBYTE16(cpu->register_X));
	}
}

void STY(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(index_size(cpu) == 8)
	{
		DB_write(memory, addr, LE_LBYTE16(cpu->register_Y));
	}
	else 
	{
		DB_write(memory, addr, LE_LBYTE16(cpu->register_Y));
		DB_write(memory, addr + 1, LE_HBYTE16(cpu->register_Y));
	}
}

void STZ(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		DB_write(memory, addr, 0x00);
	}
	else 
	{
		DB_write(memory, addr, 0x00);
		DB_write(memory, addr + 1, 0x00);
	}
}

void TAX(struct Ricoh_5A22 *cpu)
{
	// interal OP = +1 cycles
	if(index_size(cpu) == 8)
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(LE_LBYTE16(cpu->register_A), 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (LE_LBYTE16(cpu->register_A) == 0x00));

		cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, LE_LBYTE16(cpu->register_A));
	}
	else
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->register_A, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->register_A == 0x0000));

		cpu->register_X = cpu->register_A;
	}
}

void TAY(struct Ricoh_5A22 *cpu)
{
	// interal OP = +1 cycles
	if(index_size(cpu) == 8)
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(LE_LBYTE16(cpu->register_A), 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (LE_LBYTE16(cpu->register_A) == 0x00));

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, LE_LBYTE16(cpu->register_A));
	}
	else
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->register_A, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->register_A == 0x0000));

		cpu->register_Y = cpu->register_A;
	}
}

void TCD(struct Ricoh_5A22 *cpu)
{
	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->register_A, 0x8000));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->register_A == 0x0000));

	cpu->direct_page = cpu->register_A;
}

void TCS(struct Ricoh_5A22 *cpu)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(LE_LBYTE16(cpu->register_A), 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (LE_LBYTE16(cpu->register_A) == 0x00));

		cpu->stack_ptr = SWP_LE_LBYTE16(cpu->stack_ptr, LE_LBYTE16(cpu->register_A));
		cpu->stack_ptr = SWP_LE_HBYTE16(cpu->stack_ptr, 0x01);
	}
	else 
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->register_A, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->register_A == 0x0000));

		cpu->stack_ptr = cpu->register_A;
	}
}

void TDC(struct Ricoh_5A22 *cpu)
{
	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->direct_page, 0x8000));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->direct_page == 0x0000));

	cpu->register_A = cpu->direct_page;
}

void TRB(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t reset = operand & (~get_A(cpu));
		uint8_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		DB_write(memory, addr, reset);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t reset = operand & (~get_A(cpu));
		uint16_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		DB_write(memory, addr, LE_LBYTE16(reset));
		DB_write(memory, addr + 1, LE_HBYTE16(reset));
	}
}

void TSB(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t reset = operand | get_A(cpu);
		uint8_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		DB_write(memory, addr, reset);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t reset = operand | get_A(cpu);
		uint16_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		DB_write(memory, addr, LE_LBYTE16(reset));
		DB_write(memory, addr + 1, LE_HBYTE16(reset));
	}
}

void TSC(struct Ricoh_5A22 *cpu)
{
	cpu->register_A = get_SP(cpu);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(get_SP(cpu), 0x8000));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (get_SP(cpu) == 0x0000));
}

void TSX(struct Ricoh_5A22 *cpu)
{
	if(index_size(cpu) == 8)
	{
		uint8_t sp = LE_LBYTE16(get_SP(cpu));

		cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, sp);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(sp, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (sp == 0x00));
	}
	else 
	{
		uint8_t sp = get_SP(cpu);

		cpu->register_X = sp;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(sp, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (sp == 0x0000));
	}
}

void TXA(struct Ricoh_5A22 *cpu)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t x = LE_LBYTE16(cpu->register_X);

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, x);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(x, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (x == 0x00));
	}
	else 
	{
		uint16_t x = cpu->register_X;

		cpu->register_A = x;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(x, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (x == 0x0000));
	}
}

void TXS(struct Ricoh_5A22 *cpu)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(LE_LBYTE16(cpu->register_X), 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (LE_LBYTE16(cpu->register_X) == 0x00));

		cpu->stack_ptr = SWP_LE_LBYTE16(cpu->stack_ptr, LE_LBYTE16(cpu->register_X));
		cpu->stack_ptr = SWP_LE_HBYTE16(cpu->stack_ptr, 0x01);
	}
	else 
	{
		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->register_X, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->register_X == 0x0000));

		cpu->stack_ptr = cpu->register_X;
	}
}

void TXY(struct Ricoh_5A22 *cpu)
{
	if(index_size(cpu) == 8)
	{
		uint8_t x = get_X(cpu);

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, x);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(x, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (x == 0x00));
	}
	else 
	{
		uint16_t x = get_X(cpu);

		cpu->register_Y = x;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(x, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (x == 0x0000));
	}
}

void TYA(struct Ricoh_5A22 *cpu)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t y = LE_LBYTE16(cpu->register_Y);

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, y);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(y, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (y == 0x00));
	}
	else 
	{
		uint16_t y = cpu->register_Y;

		cpu->register_A = y;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(y, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (y == 0x0000));
	}
}

void TYX(struct Ricoh_5A22 *cpu)
{
	if(index_size(cpu) == 8)
	{
		uint8_t y = get_Y(cpu);

		cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, y);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(y, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (y == 0x00));
	}
	else 
	{
		uint16_t y = get_Y(cpu);

		cpu->register_X = y;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(y, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (y == 0x0000));
	}
}

void WAI(struct Ricoh_5A22 *cpu)
{
	// takes 2 cycles like normal, but the last cycle is to halt the program
	cpu->RDY = 0;
}

void WDM()
{
	// No operation

	return;
}

void XBA(struct Ricoh_5A22 *cpu)
{
	uint8_t A = LE_LBYTE16(cpu->register_A);
	uint8_t B = LE_HBYTE16(cpu->register_A);

	cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, B);
	cpu->register_A = SWP_LE_HBYTE16(cpu->register_A, A);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(LE_LBYTE16(cpu->register_A), 0x80));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (LE_LBYTE16(cpu->register_A) == 0x00));
}

void XCE(struct Ricoh_5A22 *cpu)
{
	uint8_t C = cpu->cpu_status & CPU_STATUS_C;
	uint8_t E = cpu->cpu_emulation6502 & CPU_STATUS_E;

	cpu->cpu_status = (cpu->cpu_status & (~CPU_STATUS_E)) | E;
	cpu->cpu_emulation6502 = (cpu->cpu_emulation6502 & (~CPU_STATUS_C)) | C;
}

void decode_execute(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t opcode_addr = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	cpu->program_ctr++;

	switch(DB_read(memory, opcode_addr))
	{
		uint32_t data_addr;

		//
		// ADC
		//
		case OPCODE_ADC_ABS:
			data_addr = addr_ABS(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR:
			data_addr = addr_DIR(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_IMM:
			data_addr = addr_IMM_M(cpu);
			printf("%06x\n", data_addr);
			ADC(cpu, memory, data_addr);

			break;	
		//
		// AND
		//
		case OPCODE_AND_ABS:
			data_addr = addr_ABS(cpu, memory);
			AND(cpu, memory, data_addr);

			break;	
		case OPCODE_AND_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR:
			data_addr = addr_DIR(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_IMM:
			data_addr = addr_IMM_M(cpu);
			AND(cpu, memory, data_addr);

			break;
		//
		// ASL
		//
		case OPCODE_ASL_ABS:
			data_addr = addr_ABS(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		case OPCODE_ASL_ACC:
			ASL_A(cpu);

			break;
		case OPCODE_ASL_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		case OPCODE_ASL_DIR:
			data_addr = addr_DIR(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		case OPCODE_ASL_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		//
		// BCC
		//
		case OPCODE_BCC_REL:
			data_addr = addr_REL(cpu);
			BCC(cpu, memory, data_addr);

			break;
		//
		// BCS
		//
		case OPCODE_BCS_REL:
			data_addr = addr_REL(cpu);
			BCS(cpu, memory, data_addr);

			break;
		//
		// BEQ
		//
		case OPCODE_BEQ_REL:
			data_addr = addr_REL(cpu);
			BEQ(cpu, memory, data_addr);

			break;
		//
		// BIT
		//
		case OPCODE_BIT_ABS:
			data_addr = addr_ABS(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_DIR:
			data_addr = addr_DIR(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_IMM:
			data_addr = addr_IMM_M(cpu);
			BIT_IMM(cpu, memory, data_addr);

			break;
		//
		// BMI
		//
		case OPCODE_BMI_REL:
			data_addr = addr_REL(cpu);
			BMI(cpu, memory, data_addr);

			break;
		//
		// BNE
		//
		case OPCODE_BNE_REL:
			data_addr = addr_REL(cpu);
			BNE(cpu, memory, data_addr);

			break;
		//
		// BPL
		//
		case OPCODE_BPL_REL:
			data_addr = addr_REL(cpu);
			BPL(cpu, memory, data_addr);

			break;
		//
		// BRA
		//
		case OPCODE_BRA_REL:
			data_addr = addr_REL(cpu);
			BRA(cpu, memory, data_addr);

			break;
		//
		// BRK
		//
		case OPCODE_BRK_STK:
			BRK(cpu, memory);

			break;
		//
		// BRL
		//
		case OPCODE_BRL_REL_L:
			data_addr = addr_REL_L(cpu);
			BRL(cpu, memory, data_addr);

			break;
		//
		// BVC
		//
		case OPCODE_BVC_REL:
			data_addr = addr_REL(cpu);
			BVC(cpu, memory, data_addr);

			break;
		//
		// BVS
		//
		case OPCODE_BVS_REL:
			data_addr = addr_REL(cpu);
			BVS(cpu, memory, data_addr);

			break;
		//
		// CLC
		//
		case OPCODE_CLC_IMP:
			CLC(cpu);

			break;
		//
		// CLD
		//
		case OPCODE_CLD_IMP:
			CLD(cpu);

			break;
		//
		// CLI
		//
		case OPCODE_CLI_IMP:
			CLI(cpu);

			break;
		//
		// CLV
		//
		case OPCODE_CLV_IMP:
			CLV(cpu);

			break;
		//
		// CMP
		//
		case OPCODE_CMP_ABS:
			data_addr = addr_ABS(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR:
			data_addr = addr_DIR(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_IMM:
			data_addr = addr_IMM_M(cpu);
			CMP(cpu, memory, data_addr);

			break;
		//
		// COP
		//
		case OPCODE_COP_STK:
			COP(cpu, memory);

			break;
		//
		// CPX
		//
		case OPCODE_CPX_ABS:
			data_addr = addr_ABS(cpu, memory);
			CPX(cpu, memory, data_addr);

			break;
		case OPCODE_CPX_DIR:
			data_addr = addr_DIR(cpu, memory);
			CPX(cpu, memory, data_addr);

			break;
		case OPCODE_CPX_IMM:
			data_addr = addr_IMM_X(cpu);
			CPX(cpu, memory, data_addr);

			break;
		//
		// CPY
		//
		case OPCODE_CPY_ABS:
			data_addr = addr_ABS(cpu, memory);
			CPY(cpu, memory, data_addr);

			break;
		case OPCODE_CPY_DIR:
			data_addr = addr_DIR(cpu, memory);
			CPY(cpu, memory, data_addr);

			break;
		case OPCODE_CPY_IMM:
			data_addr = addr_IMM_X(cpu);
			CPY(cpu, memory, data_addr);

			break;
		//
		// DEC
		//
		case OPCODE_DEC_ABS:
			data_addr = addr_ABS(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		case OPCODE_DEC_ACC:
			DEC_A(cpu);

			break;
		case OPCODE_DEC_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		case OPCODE_DEC_DIR:
			data_addr = addr_DIR(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		case OPCODE_DEC_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		//
		// DEX
		//
		case OPCODE_DEX_IMP:
			DEX(cpu);

			break;
		//
		// DEY
		//
		case OPCODE_DEY_IMP:
			DEY(cpu);

			break;
		//
		// EOR
		//
		case OPCODE_EOR_ABS:
			data_addr = addr_ABS(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR:
			data_addr = addr_DIR(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_IMM:
			data_addr = addr_IMM_M(cpu);
			EOR(cpu, memory, data_addr);

			break;
		//
		// INC
		//
		case OPCODE_INC_ABS:
			data_addr = addr_ABS(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		case OPCODE_INC_ACC:
			INC_A(cpu);

			break;
		case OPCODE_INC_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		case OPCODE_INC_DIR:
			data_addr = addr_DIR(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		case OPCODE_INC_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		//
		// INX
		//
		case OPCODE_INX_IMP:
			INX(cpu);
			
			break;
		//
		// INY
		//
		case OPCODE_INY_IMP:
			INY(cpu);
			
			break;
		//
		// JML
		//
		case OPCODE_JML_ABS_IL:
			data_addr = addr_ABS_IL(cpu, memory);
			JMP(cpu, data_addr);

			break;
		case OPCODE_JML_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			JMP(cpu, data_addr);

			break;
		//
		// JMP
		//
		case OPCODE_JMP_ABS:
			data_addr = addr_ABS(cpu, memory);
			JMP(cpu, data_addr);

			break;
		case OPCODE_JMP_ABS_I:
			data_addr = addr_ABS_I(cpu, memory);
			JMP(cpu, data_addr);

			break;
		case OPCODE_JMP_ABS_II:
			data_addr = addr_ABS_II(cpu, memory);
			JMP(cpu, data_addr);

			break;
		//
		// JSL
		//
		case OPCODE_JSL_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			JSL(cpu, memory, data_addr);

			break;
		//
		// JSR
		//
		case OPCODE_JSR_ABS:
			data_addr = addr_ABS(cpu, memory);
			JSR(cpu, memory, data_addr);

			break;
		case OPCODE_JSR_ABS_II:
			data_addr = addr_ABS_II(cpu, memory);
			JSR(cpu, memory, data_addr);

			break;
		//
		// LDA
		//
		case OPCODE_LDA_ABS:
			data_addr = addr_ABS(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR:
			data_addr = addr_DIR(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_IMM:
			data_addr = addr_IMM_M(cpu);
			LDA(cpu, memory, data_addr);

			break;
		//
		// LDX
		//
		case OPCODE_LDX_ABS:
			data_addr = addr_ABS(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_DIR:
			data_addr = addr_DIR(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_DIR_IY:
			data_addr = addr_DIR_IY(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_IMM:
			data_addr = addr_IMM_X(cpu);
			LDX(cpu, memory, data_addr);

			break;
		//
		// LDY
		//
		case OPCODE_LDY_ABS:
			data_addr = addr_ABS(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_DIR:
			data_addr = addr_DIR(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_IMM:
			data_addr = addr_IMM_X(cpu);
			LDY(cpu, memory, data_addr);

			break;
		//
		// LSR
		//
		case OPCODE_LSR_ABS:
			data_addr = addr_ABS(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		case OPCODE_LSR_ACC:
			LSR_A(cpu);

			break;
		case OPCODE_LSR_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		case OPCODE_LSR_DIR:
			data_addr = addr_DIR(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		case OPCODE_LSR_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		//
		// MVN
		//
		case OPCODE_MVN_XYC:
			data_addr = addr_XYC(cpu);
			MVN(cpu, memory, data_addr);

			break;
		//
		// MVP
		//
		case OPCODE_MVP_XYC:
			data_addr = addr_XYC(cpu);
			MVP(cpu, memory, data_addr);

			break;
		//
		// NOP
		//
		case OPCODE_NOP_IMP:
			NOP();

			break;
		//
		// ORA
		//
		case OPCODE_ORA_ABS:
			data_addr = addr_ABS(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR:
			data_addr = addr_DIR(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_IMM:
			data_addr = addr_IMM_M(cpu);
			ORA(cpu, memory, data_addr);

			break;
		//
		// PEA
		//
		case OPCODE_PEA_STK:
			data_addr = addr_ABS(cpu, memory);
			PEA(cpu, memory, data_addr);
			
			break;
		//
		// PEI
		//
		case OPCODE_PEI_STK:
			data_addr = addr_DIR_I(cpu, memory);
			PEI(cpu, memory, data_addr);
			
			break;
		//
		// PER
		//
		case OPCODE_PER_STK:
			data_addr = addr_REL_L(cpu);
			PEA(cpu, memory, data_addr);
			
			break;
		//
		// PHA
		//
		case OPCODE_PHA_STK:
			PHA(cpu, memory);

			break;
		//
		// PHB
		//
		case OPCODE_PHB_STK:
			PHB(cpu, memory);

			break;
		//
		// PHD
		//
		case OPCODE_PHD_STK:
			PHD(cpu, memory);

			break;
		//
		// PHK
		//
		case OPCODE_PHK_STK:
			PHK(cpu, memory);

			break;
		//
		// PHP
		//
		case OPCODE_PHP_STK:
			PHP(cpu, memory);

			break;
		//
		// PHX
		//
		case OPCODE_PHX_STK:
			PHX(cpu, memory);

			break;
		//
		// PHY
		//
		case OPCODE_PHY_STK:
			PHY(cpu, memory);

			break;
		//
		// PLA
		//
		case OPCODE_PLA_STK:
			PLA(cpu, memory);

			break;
		//
		// PLB
		//
		case OPCODE_PLB_STK:
			PLB(cpu, memory);

			break;
		//
		// PLD
		//
		case OPCODE_PLD_STK:
			PLD(cpu, memory);

			break;
		//
		// PLP
		//
		case OPCODE_PLP_STK:
			PLP(cpu, memory);

			break;
		//
		// PLX
		//
		case OPCODE_PLX_STK:
			PLX(cpu, memory);

			break;
		//
		// PLY
		//
		case OPCODE_PLY_STK:
			PLY(cpu, memory);

			break;
		//
		// REP
		//
		case OPCODE_REP_IMM:
			data_addr = addr_IMM_8(cpu);
			REP(cpu, memory, data_addr);

			break;
		//
		// ROL
		//
		case OPCODE_ROL_ABS:
			data_addr = addr_ABS(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		case OPCODE_ROL_ACC:
			ROL_A(cpu, memory);

			break;
		case OPCODE_ROL_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		case OPCODE_ROL_DIR:
			data_addr = addr_DIR(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		case OPCODE_ROL_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		//
		// ROR
		//
		case OPCODE_ROR_ABS:
			data_addr = addr_ABS(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		case OPCODE_ROR_ACC:
			ROR_A(cpu, memory);

			break;
		case OPCODE_ROR_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		case OPCODE_ROR_DIR:
			data_addr = addr_DIR(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		case OPCODE_ROR_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		//
		// RTI
		//
		case OPCODE_RTI_STK:
			RTI(cpu, memory);

			break;
		//
		// RTL
		//
		case OPCODE_RTL_STK:
			RTL(cpu, memory);

			break;
		//
		// RTS
		//
		case OPCODE_RTS_STK:
			RTS(cpu, memory);

			break;
		//
		// SBC
		//
		case OPCODE_SBC_ABS:
			data_addr = addr_ABS(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_ABS_IIY:
			data_addr = addr_ABS_IIY(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR:
			data_addr = addr_DIR(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_IMM:
			data_addr = addr_IMM_M(cpu);
			SBC(cpu, memory, data_addr);

			break;
		//
		// SEC
		//
		case OPCODE_SEC_IMP:
			SEC(cpu);

			break;
		//
		// SED
		//
		case OPCODE_SED_IMP:
			SED(cpu);

			break;
		//
		// SEI
		//
		case OPCODE_SEI_IMP:
			SEI(cpu);

			break;
		//
		// SEP
		//
		case OPCODE_SEP_IMM:
			data_addr = addr_IMM_8(cpu);
			SEP(cpu, memory, data_addr);

			break;
		//
		// STA
		//
		case OPCODE_STA_ABS:
			data_addr = addr_ABS(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_ABS_L:
			data_addr = addr_ABS_L(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_ABS_LIX:
			data_addr = addr_ABS_LIX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR:
			data_addr = addr_DIR(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_STK_R:
			data_addr = addr_STK_R(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_I:
			data_addr = addr_DIR_I(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IL:
			data_addr = addr_DIR_IL(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_STK_RII:
			data_addr = addr_STK_RII(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IIX:
			data_addr = addr_DIR_IIX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IIY:
			data_addr = addr_DIR_IIY(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_ILI:
			data_addr = addr_DIR_ILI(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		//
		// STP
		//
		case OPCODE_STP_IMP:
			STP(cpu);

			break;
		//
		// STX
		//
		case OPCODE_STX_ABS:
			data_addr = addr_ABS(cpu, memory);
			STX(cpu, memory, data_addr);

			break;
		case OPCODE_STX_DIR:
			data_addr = addr_DIR(cpu, memory);
			STX(cpu, memory, data_addr);

			break;
		case OPCODE_STX_DIR_IY:
			data_addr = addr_DIR_IY(cpu, memory);
			STX(cpu, memory, data_addr);

			break;
		//
		// STY
		//
		case OPCODE_STZ_ABS:
			data_addr = addr_ABS(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		case OPCODE_STZ_ABS_IIX:
			data_addr = addr_ABS_IIX(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		case OPCODE_STZ_DIR:
			data_addr = addr_DIR(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		case OPCODE_STZ_DIR_IX:
			data_addr = addr_DIR_IX(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		//
		// TAX
		//
		case OPCODE_TAX_IMP:
			TAX(cpu);

			break;
		//
		// TAY
		//
		case OPCODE_TAY_IMP:
			TAY(cpu);

			break;
		//
		// TCD
		//
		case OPCODE_TCD_IMP:
			TCD(cpu);

			break;
		//
		// TCS
		//
		case OPCODE_TCS_IMP:
			TCS(cpu);

			break;
		//
		// TDC
		//
		case OPCODE_TDC_IMP:
			TDC(cpu);

			break;
		//
		// TRB
		//
		case OPCODE_TRB_ABS:
			data_addr = addr_ABS(cpu, memory);
			TRB(cpu, memory, data_addr);

			break;
		case OPCODE_TRB_DIR:
			data_addr = addr_DIR(cpu, memory);
			TRB(cpu, memory, data_addr);

			break;
		//
		// TSB
		//
		case OPCODE_TSB_ABS:
			data_addr = addr_ABS(cpu, memory);
			TSB(cpu, memory, data_addr);

			break;
		case OPCODE_TSB_DIR:
			data_addr = addr_DIR(cpu, memory);
			TSB(cpu, memory, data_addr);

			break;
		//
		// TSC
		//
		case OPCODE_TSC_IMP:
			TSC(cpu);

			break;
		//
		// TSX
		//
		case OPCODE_TSX_IMP:
			TSX(cpu);

			break;
		//
		// TXA
		//
		case OPCODE_TXA_IMP:
			TXA(cpu);

			break;
		//
		// TXS
		//
		case OPCODE_TXS_IMP:
			TXS(cpu);

			break;
		//
		// TXY
		//
		case OPCODE_TXY_IMP:
			TXY(cpu);

			break;
		//
		// TYA
		//
		case OPCODE_TYA_IMP:
			TYA(cpu);

			break;
		//
		// TYX
		//
		case OPCODE_TYX_IMP:
			TYX(cpu);

			break;
		//
		// WAI
		//
		case OPCODE_WAI_IMP:
			WAI(cpu);

			break;
		//
		// WDM
		//
		case OPCODE_WDM_IMM:
			data_addr = addr_IMM_8(cpu);
			WDM();

			break;
		//
		// XBA
		//
		case OPCODE_XBA_IMP:
			XBA(cpu);

			break;
		//
		// XCE
		//
		case OPCODE_XCE_IMP:
			XCE(cpu);

			break;
		default:
			break;
	}
}

void reset_ricoh_5a22(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	cpu->data_bank = 0x00;
	cpu->program_bank = 0x00;
	cpu->direct_page = 0x0000;

	cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(memory, RESET_VECTOR_6502[0]), DB_read(memory, RESET_VECTOR_6502[1]));

	cpu->cpu_status = 0b00000000;
	cpu->cpu_emulation6502 = 0b00000000;

	cpu->cpu_emulation6502 = cpu->cpu_emulation6502 | CPU_STATUS_E;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_M;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_I;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_X;

	cpu->RDY = 1;
	cpu->LPM = 0;

	cpu->NMI_line = 0;
	cpu->IRQ_line = 0;
}

