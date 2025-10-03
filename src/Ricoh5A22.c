#include "Ricoh5A22.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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

uint8_t bus_read(uint8_t *addr)
{
	return *addr;
}

uint32_t addr_ABS(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, memory[oper], memory[oper + 1]);

	cpu->program_ctr += 2;

	printf("%02x %02x %02x %06x\n", cpu->data_bank, memory[oper], memory[oper + 1], addr);

	return addr;
}

uint32_t addr_ABS_IIX(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, memory[oper], memory[oper + 1]) + cpu->register_X;

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_IIY(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, memory[oper], memory[oper + 1]) + cpu->register_Y;

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_L(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_3BYTE(memory[oper], memory[oper + 1], memory[oper + 2]);

	cpu->program_ctr += 3;

	return addr;
}

uint32_t addr_ABS_LIX(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_3BYTE(memory[oper], memory[oper + 1], memory[oper + 2]) + cpu->register_X;

	cpu->program_ctr += 3;

	return addr;
}

uint32_t addr_ABS_I(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_2BYTE(memory[oper], memory[oper + 1]);
	uint32_t addr_indirect = LE_COMBINE_2BYTE(memory[addr], memory[addr + 1]);
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->program_bank, addr_indirect);

	cpu->program_ctr += 2;

	return addr_effective;
}

uint32_t addr_ABS_II(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_2BYTE(memory[oper], memory[oper + 1]) + cpu->register_X;
	uint32_t addr_indirect = LE_COMBINE_2BYTE(memory[addr], memory[addr + 1]);
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->program_bank, addr_indirect);

	cpu->program_ctr += 2;

	return addr_effective;
}

uint32_t addr_DIR(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + memory[oper];

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_STK_R(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->stack_ptr + memory[oper];

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IX(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + memory[oper] + cpu->register_X;

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IY(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = cpu->direct_page + memory[oper] + cpu->register_Y;

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_I(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

uint32_t addr_DIR_IL(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

uint32_t addr_STK_RII(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

uint32_t addr_DIR_IIX(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

uint32_t addr_DIR_IIY(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

uint32_t addr_DIR_ILI(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

uint32_t addr_REL_L(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

uint32_t addr_IMM(struct Ricoh_5A22 *cpu)
{
	return 0x00000000;
}

void add_with_carry(struct Ricoh_5A22 *cpu, uint32_t operand)
{
	if(check_bit8(cpu->cpu_status, CPU_STATUS_M)) // 8 bit
	{
		uint8_t converted_operand = operand & 0x000000FF;
		uint8_t converted_accumulator = cpu->register_A & 0x000000FF;

		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint8_t cin = check_bit8(cpu->cpu_status, CPU_STATUS_C);

			uint8_t opr_digit_0 = converted_operand & 0x0F;
			uint8_t acc_digit_0 = converted_accumulator & 0x0F;

			uint8_t opr_digit_1 = (converted_operand & 0xF0) >> 4;
			uint8_t acc_digit_1 = (converted_accumulator & 0xF0) >> 4;

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

				if(check_bit8(coutl_1, 0x40))
				{
					cpu->cpu_status |= CPU_STATUS_V;
				}

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

			couth_1 = couth_1 << 1;

			while(couth_1 != 0)
			{
				sumh_0 = sumh_1;
				couth_0 = couth_1;

				sumh_1 = sumh_0 ^ couth_0;
				couth_1 = sumh_0 & couth_0;

				if(check_bit8(couth_1, 0x40))
				{
					cpu->cpu_status |= CPU_STATUS_V;
				}

				couth_1 = couth_1 << 1;
			}

			if(sumh_1 > 0x09)
			{
				sumh_1 += 0x06;
				cpu->cpu_status |= CPU_STATUS_C;
			}
			else 
			{
				cin = 0x00;
			}

			suml_1 &= 0x0F;
			sumh_1 &= 0x0F;

			cpu->register_A = cpu->register_A & 0xFF00;
			cpu->register_A = ((cpu->register_A | sumh_1) << 4) | suml_1;
		}
		else 
		{
			uint8_t cin = check_bit8(cpu->cpu_status, CPU_STATUS_C);

			uint8_t sum_0 = converted_accumulator ^ converted_operand;
			uint8_t cout_0 = converted_accumulator & converted_operand;

			uint8_t sum_1 = sum_0 ^ cin;
			uint8_t cout_1 = (sum_0 & cin) | cout_0;

			if(check_bit8(cout_1, 0x80))
			{
				cpu->cpu_status |= CPU_STATUS_C;
			}

			if(check_bit8(cout_1, 0x40))
			{
				cpu->cpu_status |= CPU_STATUS_V;
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
					cpu->cpu_status |= CPU_STATUS_C;
				}

				if(check_bit8(cout_1, 0x40))
				{
					cpu->cpu_status |= CPU_STATUS_V;
				}

				cout_1 = cout_1 << 1;
			}

			if(check_bit8(cout_1, 0x80))
			{
				cpu->cpu_status |= CPU_STATUS_N;
			}

			if(sum_1 == 0)
			{
				cpu->cpu_status |= CPU_STATUS_Z;
			}

			cpu->register_A = cpu->register_A & 0xFF00;
			cpu->register_A = cpu->register_A | sum_1;
		}
	}
	else  // 16 bit
	{
		uint16_t converted_operand = operand & 0x0000FFFF;
		uint16_t converted_accumulator = cpu->register_A & 0x0000FFFF;

		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint16_t cin = (uint16_t)check_bit8(cpu->cpu_status, CPU_STATUS_C);

			uint8_t opr_digit_0 = converted_operand & 0x000F;
			uint8_t acc_digit_0 = converted_accumulator & 0x000F;

			uint8_t opr_digit_1 = (converted_operand & 0x00F0) >> 4;
			uint8_t acc_digit_1 = (converted_accumulator & 0x00F0) >> 4;

			uint8_t opr_digit_2 = (converted_operand & 0x0F00) >> 8;
			uint8_t acc_digit_2 = (converted_accumulator & 0x0F00) >> 8;

			uint8_t opr_digit_3 = (converted_operand & 0xF000) >> 12;
			uint8_t acc_digit_3 = (converted_accumulator & 0xF000) >> 12;

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

			coutbh_1 = coutbh_1 << 1;

			while(coutbh_1 != 0)
			{
				sumbh_0 = sumbh_1;
				coutbh_0 = coutbh_1;

				sumbh_1 = sumbh_0 ^ coutbh_0;
				coutbh_1 = sumbh_0 & coutbh_0;

				if(check_bit8(coutbh_1, 0x40))
				{
					cpu->cpu_status |= CPU_STATUS_V;
				}

				coutbh_1 = coutbh_1 << 1;
			}

			if(sumbh_1 > 0x09)
			{
				cpu->cpu_status |= CPU_STATUS_C;
				sumbh_1 += 0x06;
			}
			else 
			{
				cin = 0x00;
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

			uint16_t sum_0 = converted_accumulator ^ converted_operand;
			uint16_t cout_0 = converted_accumulator & converted_operand;

			uint16_t sum_1 = sum_0 ^ cin;
			uint16_t cout_1 = (sum_0 & cin) | cout_0;

			if(check_bit16(cout_1, 0x8000))
			{
				cpu->cpu_status |= CPU_STATUS_C;
			}

			if(check_bit16(cout_1, 0x4000))
			{
				cpu->cpu_status |= CPU_STATUS_V;
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
					cpu->cpu_status |= CPU_STATUS_C;
				}

				if(check_bit16(cout_1, 0x4000))
				{
					cpu->cpu_status |= CPU_STATUS_V;
				}

				cout_1 = cout_1 << 1;
			}

			if(check_bit16(cout_1, 0x8000))
			{
				cpu->cpu_status |= CPU_STATUS_N;
			}

			if(sum_1 == 0)
			{
				cpu->cpu_status |= CPU_STATUS_Z;
			}

			cpu->register_A = sum_1;
		}
	}
}

void decode_execute(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	uint32_t opcode_addr = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	cpu->program_ctr++;

	switch(memory[opcode_addr])
	{
		case OPCODE_ADC_ABS:
			{
				uint32_t operand_addr = addr_ABS(cpu, memory);
				uint32_t operand = LE_COMBINE_2BYTE(memory[operand_addr], memory[operand_addr + 1]);

				printf("%08x %08x\n", operand_addr, operand);

				add_with_carry(cpu, operand);
			}

			break;
		default:
			break;
	}
}

void init_ricoh_5a22(struct Ricoh_5A22 *cpu, uint8_t memory[])
{
	cpu->data_bank = 0x00;
	cpu->program_bank = 0x00;
	cpu->direct_page = 0x0000;

	cpu->program_ctr = LE_COMBINE_2BYTE(memory[RESET_VECTOR_6502[0]], memory[RESET_VECTOR_6502[1]]);

	cpu->cpu_status = 0b00000000;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_M;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_I;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_X;
}

