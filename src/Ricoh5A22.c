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

int accumulator_size(struct Ricoh_5A22 *cpu)
{
	if(check_bit8(cpu->cpu_status, CPU_STATUS_E) || check_bit8(cpu->cpu_status, CPU_STATUS_M))
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

	if(check_bit8(cpu->cpu_status, CPU_STATUS_E) || check_bit8(cpu->cpu_status, CPU_STATUS_X))
	{
		return 8;
	}
	else 
	{
		return 16;
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
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, oper), DB_read(memory, oper + 1)) + cpu->register_X;

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_IIY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, oper), DB_read(memory, oper + 1)) + cpu->register_Y;

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
	uint32_t addr = LE_COMBINE_3BYTE(DB_read(memory, oper), DB_read(memory, oper + 1), DB_read(memory, oper + 2)) + cpu->register_X;

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

uint32_t addr_ABS_II(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_2BYTE(DB_read(memory, oper), DB_read(memory, oper + 1)) + cpu->register_X;
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->program_bank, addr_indirect);

	cpu->program_ctr += 2;

	return addr_effective;
}

uint32_t addr_DIR(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_STK_R(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->stack_ptr + DB_read(memory, oper);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper) + cpu->register_X;

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper) + cpu->register_Y;

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_I(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->data_bank, addr_indirect);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IL(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_effective = LE_COMBINE_3BYTE(DB_read(memory, addr), DB_read(memory, addr + 1), DB_read(memory, addr + 2));

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_STK_RII(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->stack_ptr + DB_read(memory, oper);
	uint32_t addr_base = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = addr_base + cpu->register_Y;

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IIX(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper) + cpu->register_X;
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->data_bank, addr_indirect);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IIY(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_indirect = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(memory, addr), DB_read(memory, addr + 1));
	uint32_t addr_effective = addr_indirect + cpu->register_Y;

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_ILI(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + DB_read(memory, oper);
	uint32_t addr_indirect = LE_COMBINE_3BYTE(DB_read(memory, addr), DB_read(memory, addr + 1), DB_read(memory, addr + 2));
	uint32_t addr_effective = addr_indirect + cpu->register_Y;

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

void ADC(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	bool carry = false;
	bool negative = false;
	bool overflow = false;
	bool zero = false;

	if(accumulator_size(cpu) == 8) // 8 bit
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = LE_LBYTE16(cpu->register_A);

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

			cpu->register_A = cpu->register_A & 0xFF00;
			cpu->register_A = ((cpu->register_A | sumh_1) << 4) | suml_1;
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

			cpu->register_A = cpu->register_A & 0xFF00;
			cpu->register_A = cpu->register_A | sum_1;
		}
	}
	else  // 16 bit
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = cpu->register_A;

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
		uint8_t accumulator = LE_LBYTE16(cpu->register_A);

		uint8_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x40));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = (cpu->register_A & 0xFF00) | result;
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = cpu->register_A;

		uint16_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x4000));
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

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x40));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		DB_write(memory, addr, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x4000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		DB_write(memory, addr, LE_LBYTE16(operand));
		DB_write(memory, addr + 1, LE_HBYTE16(operand));
	}
}

void ASL_A(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = LE_LBYTE16(cpu->register_A);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x80));
		
		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x40));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A &= 0xFF00;
		cpu->register_A |= operand;
	}
	else 
	{
		uint16_t operand = cpu->register_A;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x4000));
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
		uint8_t accumulator = cpu->register_A & 0x00FF;

		uint8_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit8(operand, 0x40));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = cpu->register_A;

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
		uint8_t accumulator = cpu->register_A & 0x00FF;

		uint8_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = cpu->register_A;

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

void BRK(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		DB_write(memory, addr, LE_HBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 1, LE_LBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 2, cpu->cpu_status);

		cpu->stack_ptr -= 3;

		cpu->cpu_status |= CPU_STATUS_B;
		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(BRK_VECTOR_65816[0], BRK_VECTOR_65816[1]);
	}
	else 
	{
		DB_write(memory, addr, cpu->program_bank);
		DB_write(memory, addr - 1, LE_HBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 2, LE_LBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 3, cpu->cpu_status);

		cpu->stack_ptr -= 4;

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
		uint8_t accumulator = LE_LBYTE16(cpu->register_A);

		uint8_t result = accumulator - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = cpu->register_A;

		uint16_t result = accumulator - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
}

void COP(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		DB_write(memory, addr, LE_HBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 1, LE_LBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 2, cpu->cpu_status);

		cpu->stack_ptr -= 3;

		cpu->cpu_status |= CPU_STATUS_B;
		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(COP_VECTOR_65816[0], COP_VECTOR_65816[1]);
	}
	else 
	{
		DB_write(memory, addr, cpu->program_bank);
		DB_write(memory, addr - 1, LE_HBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 2, LE_LBYTE16(cpu->program_ctr));
		DB_write(memory, addr - 3, cpu->cpu_status);

		cpu->stack_ptr -= 4;

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
		uint8_t index = LE_LBYTE16(cpu->register_X);

		uint8_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t index = cpu->register_X;

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
		uint8_t index = LE_LBYTE16(cpu->register_Y);

		uint8_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (result >= 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t index = cpu->register_Y;

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

void DEC_A(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = LE_LBYTE16(cpu->register_A);
		
		uint8_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A &= 0xFF00;
		cpu->register_A |= result;
	}
	else 
	{
		uint16_t operand = cpu->register_A;

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
		uint8_t index = LE_LBYTE16(cpu->register_X);
		uint8_t result = index - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_X &= 0xFF00;
		cpu->register_X |= result;
	}
	else 
	{
		uint16_t index = cpu->register_X;
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
		uint8_t index = LE_LBYTE16(cpu->register_Y);
		uint8_t result = index - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_Y &= 0xFF00;
		cpu->register_Y |= result;
	}
	else 
	{
		uint16_t index = cpu->register_Y;
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
		uint8_t accumulator = LE_LBYTE16(cpu->register_A);

		uint8_t result = operand ^ accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A &= 0xFF00;
		cpu->register_A |= result;
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = cpu->register_A;

		uint16_t result = operand ^ accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
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
			ASL_A(cpu, memory);

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
			data_addr = addr_STK_R(cpu, memory);
			BRK(cpu, memory, data_addr);

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
		case OPCODE_COP_REL_L:
			data_addr = addr_STK_R(cpu, memory);
			COP(cpu, memory, data_addr);

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
			DEC_A(cpu, memory);

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

		//
		// INX
		//

		//
		// INY
		//

		//
		// JML
		//

		//
		// JMP
		//

		//
		// JSL
		//

		//
		// JSR
		//

		//
		// LDA
		//
		
		//
		// LDX
		//

		//
		// LDY
		//

		//
		// LSR
		//
	
		//
		// MVN
		//

		//
		// MVP
		//

		//
		// NOP
		//
		
		//
		// ORA
		//

		//
		// PEA
		//

		//
		// PEI
		//

		//
		// PER
		//

		//
		// PHA
		//

		//
		// PHB
		//

		//
		// PHD
		//

		//
		// PHK
		//

		//
		// PHP
		//

		//
		// PHX
		//

		//
		// PHY
		//

		//
		// PLA
		//

		//
		// PLB
		//

		//
		// PLD
		//

		//
		// PLP
		//

		//
		// PLX
		//

		//
		// PLY
		//

		//
		// REP
		//

		//
		// ROL
		//

		//
		// ROR
		//

		//
		// RTI
		//

		//
		// RTL
		//

		//
		// RTS
		//

		//
		// SBC
		//

		//
		// SEC
		//

		//
		// SED
		//

		//
		// SEI
		//

		//
		// SEP
		//

		//
		// STA
		//

		//
		// STP
		//

		//
		// STX
		//

		//
		// STY
		//

		//
		// TAX
		//

		//
		// TAY
		//

		//
		// TCD
		//

		//
		// TCS
		//

		//
		// TRB
		//

		//
		// TSB
		//

		//
		// TSC
		//

		//
		// TSX
		//

		//
		// TXA
		//

		//
		// TXS
		//

		//
		// TXY
		//

		//
		// TYA
		//

		//
		// TYX
		//

		//
		// WAI
		//

		//
		// WDM
		//

		//
		// XBA
		//

		//
		// XCE
		//
		default:
			break;
	}
}

void init_ricoh_5a22(struct Ricoh_5A22 *cpu, struct Memory *memory)
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
}

