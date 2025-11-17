#include "Ricoh5A22.h"
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
		cpu->stack_ptr = SWP_LE_HBYTE16(cpu->stack_ptr, 0x01);
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
#define get_SP(cpu) (check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) ? SWP_LE_HBYTE16(cpu->stack_ptr, 0x01) : cpu->stack_ptr)

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
	DB_write(memory, get_SP(cpu), write_val);
	decrement_SP(cpu, 1);
}

uint8_t pull_SP(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	increment_SP(cpu, 1);
	return DB_read(memory, get_SP(cpu));
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
	if(accumulator_size(cpu) == 8)
	{
		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint8_t operand = DB_read(memory, addr);
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x000000FF;

			uint32_t sum = (wide_acc & 0x0000000F) + (operand & 0x0F) + check_bit8(cpu->cpu_status, CPU_STATUS_C);

			if(sum > 0x09)
			{
				sum += 0x06;
			}

			sum += (wide_acc & 0x000000F0) + (operand & 0xF0);

			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand)) & (wide_acc ^ sum), 0x00000080));
			// BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint8_t)sum ^ get_A(cpu), 0x00000080));

			if(sum > 0x99)
			{
				sum += 0x60;
			}

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(sum, 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(sum, 0x00000100));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint8_t)sum == 0));

			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, sum & 0x000000FF);
		}
		else 
		{
			uint8_t operand = DB_read(memory, addr);
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x000000FF;

			uint32_t sum = wide_acc + operand + check_bit8(cpu->cpu_status, CPU_STATUS_C);
			
			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(sum, 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand)) & (wide_acc ^ sum), 0x00000080));
			// BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint8_t)sum ^ get_A(cpu), 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(sum, 0x00000100));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint8_t)sum == 0));

			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, sum & 0x000000FF);
		}
	}
	else 
	{
		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{

			uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x0000FFFF;

			uint32_t sum = (wide_acc & 0x0000000F) + (operand & 0x000F) + check_bit8(cpu->cpu_status, CPU_STATUS_C);
	
			if(sum > 0x0009)
			{
				sum += 0x0006;
			}

			sum += (wide_acc & 0x000000F0) + (operand & 0x00F0);

			if(sum > 0x0099)
			{
				sum += 0x0060;
			}

			sum += (wide_acc & 0x00000F00) + (operand & 0x0F00);

			if(sum > 0x0999)
			{
				sum += 0x0600;
			}

			sum += (wide_acc & 0x0000F000) + (operand & 0xF000);

			// BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint16_t)sum ^ get_A(cpu), 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand)) & (wide_acc ^ sum), 0x00008000));

			if(sum > 0x9999)
			{
				sum += 0x6000;
			}

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(sum , 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(sum , 0x00010000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint16_t)sum == 0));

			cpu->register_A = (uint16_t)(sum & 0x0000FFFF);
		}
		else 
		{
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x0000FFFF;

			uint32_t sum = wide_acc + operand + check_bit8(cpu->cpu_status, CPU_STATUS_C);

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(sum , 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand)) & (wide_acc ^ sum), 0x00008000));
			// BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((uint16_t)sum ^ get_A(cpu), 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(sum , 0x00010000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint16_t)sum == 0));

			cpu->register_A = (uint16_t)(sum & 0x00FFFF);
		}
	}
}

void AND(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(memory, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

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
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(memory, BRK_VECTOR_65816[0]), DB_read(memory, BRK_VECTOR_65816[1]));
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
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(memory, BRK_VECTOR_65816[0]), DB_read(memory, BRK_VECTOR_65816[1]));
	}
}

void BRL(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));

	cpu->program_ctr += (int16_t)operand;
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
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (accumulator >= operand));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = accumulator - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (accumulator >= operand));
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
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(memory, COP_VECTOR_65816[0]), DB_read(memory, COP_VECTOR_65816[1]));
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
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(memory, COP_VECTOR_65816[0]), DB_read(memory, COP_VECTOR_65816[1]));
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
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t index = get_X(cpu);

		uint16_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
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
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
		uint16_t index = get_Y(cpu);

		int16_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
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
	push_SP(cpu, memory, LE_HBYTE16(cpu->program_ctr - 1));
	push_SP(cpu, memory, LE_LBYTE16(cpu->program_ctr - 1));

	cpu->program_ctr = addr & 0x0000FFFF;
}

void JSL(struct Ricoh_5A22 *cpu, struct Memory *memory, uint32_t addr)
{
	push_SP(cpu, memory, cpu->program_bank);
	push_SP(cpu, memory, LE_HBYTE16(cpu->program_ctr - 1));
	push_SP(cpu, memory, LE_LBYTE16(cpu->program_ctr - 1));

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
		uint8_t acc_h = LE_HBYTE16(accumulator);
		uint8_t acc_l = LE_LBYTE16(accumulator);

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
	uint8_t direct_h = LE_HBYTE16(cpu->direct_page);
	uint8_t direct_l = LE_LBYTE16(cpu->direct_page);

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
		uint8_t index_h = LE_HBYTE16(index);
		uint8_t index_l = LE_LBYTE16(index);

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
		uint8_t index_h = LE_HBYTE16(index);
		uint8_t index_l = LE_LBYTE16(index);

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
	}
	else 
	{
		swap_cpu_status(cpu, pull_SP(cpu, memory));

		uint8_t pc_l = pull_SP(cpu, memory);
		uint8_t pc_h = pull_SP(cpu, memory);

		cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);
		cpu->program_bank = pull_SP(cpu, memory);
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
			uint32_t operand_c = 0x000000FF & (~operand);
			uint32_t wide_acc = 0x000000FF & get_A(cpu);

			uint32_t dif = wide_acc + (operand_c + 1) - (1 - check_bit8(cpu->cpu_status, CPU_STATUS_C));

			if((dif & 0x0000000F) > 0x00000009)
			{
				dif -= 0x00000006;
			}

			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand_c)) & (wide_acc ^ dif), 0x00000080));

			if((dif & 0x000000F0) > 0x00000090)
			{
				dif -= 0x00000060;
			}

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(dif, 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(dif, 0x00000100));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint8_t)dif == 0));

			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, dif & 0x000000FF);
		}
		else 
		{
			uint8_t operand = DB_read(memory, addr);
			uint32_t operand_c = ((uint32_t)(~operand) & 0x000000FF);
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x000000FF;

			uint32_t difference = wide_acc + (operand_c + 1) - (1 - check_bit8(cpu->cpu_status, CPU_STATUS_C));

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(difference, 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand_c)) & (wide_acc ^ difference), 0x00000080));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(difference, 0x00000100));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint8_t)difference == 0));

			cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, difference & 0x000000FF);
		}
	}
	else 
	{
		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
			uint32_t operand_c = 0x0000FFFF & (~operand);
			uint32_t wide_acc = 0x0000FFFF & get_A(cpu);

			uint32_t dif = wide_acc + (operand_c + 1) - (1 - check_bit8(cpu->cpu_status, CPU_STATUS_C));

			if((dif & 0x0000000F) > 0x00000009)
			{
				dif -= 0x00000006;
			}

			if((dif & 0x000000F0) > 0x00000090)
			{
				dif -= 0x00000060;
			}

			if((dif & 0x00000F00) > 0x00000900)
			{
				dif -= 0x00000600;
			}

			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand_c)) & (wide_acc ^ dif), 0x00000080));

			if((dif & 0x0000F000) > 0x00009000)
			{
				dif -= 0x00006000;
			}

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(dif, 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(dif, 0x00010000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint16_t)dif == 0));

			cpu->register_A = (uint16_t)(dif & 0x0000FFFF);
		}
		else 
		{
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(memory, addr), DB_read(memory, addr + 1));
			uint32_t operand_c = ((uint32_t)(~operand) & 0x0000FFFF);
			uint32_t wide_acc = (uint32_t)get_A(cpu) & 0x0000FFFF;

			uint32_t difference = wide_acc + (operand_c + 1) - (1 - check_bit8(cpu->cpu_status, CPU_STATUS_C));

			BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit32(difference, 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit32((~(wide_acc ^ operand_c)) & (wide_acc ^ difference), 0x00008000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit32(difference, 0x00010000));
			BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, ((uint16_t)difference == 0));

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
		cpu->stack_ptr = SWP_LE_LBYTE16(cpu->stack_ptr, LE_LBYTE16(cpu->register_A));
		cpu->stack_ptr = SWP_LE_HBYTE16(cpu->stack_ptr, 0x01);
	}
	else 
	{
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
		uint16_t sp = get_SP(cpu);

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
		cpu->stack_ptr = SWP_LE_LBYTE16(cpu->stack_ptr, LE_LBYTE16(cpu->register_X));
		cpu->stack_ptr = SWP_LE_HBYTE16(cpu->stack_ptr, 0x01);
	}
	else 
	{
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
	
	swap_cpu_status(cpu, cpu->cpu_status);
}

#define DEBUG_OPCODES 1

void decode_execute(struct Ricoh_5A22 *cpu, struct Memory *memory)
{
	uint32_t opcode_addr = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	cpu->program_ctr++;

	#if DEBUG_OPCODES
		printf("%02x ", DB_read(memory, opcode_addr));
	#endif

	switch(DB_read(memory, opcode_addr))
	{
		uint32_t data_addr;

		//
		// ADC
		//
		case OPCODE_ADC_ABS:
			#if DEBUG_OPCODES
				printf("ADC_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ADC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_IIY:
			#if DEBUG_OPCODES
				printf("ADC_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_L:
			#if DEBUG_OPCODES
				printf("ADC_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_ABS_LIX:
			#if DEBUG_OPCODES
				printf("ADC_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR:
			#if DEBUG_OPCODES
				printf("ADC_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_STK_R:
			printf("OPCODE_ADC_STK_R\n");
			data_addr = addr_STK_R(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IX:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_I:
			#if DEBUG_OPCODES
				printf("ADC_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IL:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_STK_RII:
			#if DEBUG_OPCODES
				printf("ADC_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IIX:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_IIY:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_DIR_ILI:
			#if DEBUG_OPCODES
				printf("ADC_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			ADC(cpu, memory, data_addr);

			break;
		case OPCODE_ADC_IMM:
			#if DEBUG_OPCODES
				printf("ADC_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			ADC(cpu, memory, data_addr);

			break;	
		//
		// AND
		//
		case OPCODE_AND_ABS:
			#if DEBUG_OPCODES
				printf("AND_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			AND(cpu, memory, data_addr);

			break;	
		case OPCODE_AND_ABS_IIX:
			#if DEBUG_OPCODES
				printf("AND_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_ABS_IIY:
			#if DEBUG_OPCODES
				printf("AND_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_ABS_L:
			#if DEBUG_OPCODES
				printf("AND_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_ABS_LIX:
			#if DEBUG_OPCODES
				printf("AND_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR:
			#if DEBUG_OPCODES
				printf("AND_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_STK_R:
			#if DEBUG_OPCODES
				printf("AND_STK_R\n");
			#endif
			data_addr = addr_STK_R(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IX:
			#if DEBUG_OPCODES
				printf("AND_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_I:
			#if DEBUG_OPCODES
				printf("AND_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IL:
			#if DEBUG_OPCODES
				printf("AND_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_STK_RII:
			#if DEBUG_OPCODES
				printf("AND_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IIX:
			#if DEBUG_OPCODES
				printf("AND_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_IIY:
			#if DEBUG_OPCODES
				printf("AND_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_DIR_ILI:
			#if DEBUG_OPCODES
				printf("AND_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			AND(cpu, memory, data_addr);

			break;
		case OPCODE_AND_IMM:
			#if DEBUG_OPCODES
				printf("AND_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			AND(cpu, memory, data_addr);

			break;
		//
		// ASL
		//
		case OPCODE_ASL_ABS:
			#if DEBUG_OPCODES
				printf("ASL_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		case OPCODE_ASL_ACC:
			#if DEBUG_OPCODES
				printf("ASL_ACC\n");
			#endif
			ASL_A(cpu);

			break;
		case OPCODE_ASL_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ASL_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		case OPCODE_ASL_DIR:
			#if DEBUG_OPCODES
				printf("ASL_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		case OPCODE_ASL_DIR_IX:
			#if DEBUG_OPCODES
				printf("ASL_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			ASL(cpu, memory, data_addr);

			break;
		//
		// BCC
		//
		case OPCODE_BCC_REL:
			#if DEBUG_OPCODES
				printf("BCC_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BCC(cpu, memory, data_addr);

			break;
		//
		// BCS
		//
		case OPCODE_BCS_REL:
			#if DEBUG_OPCODES
				printf("BCS_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BCS(cpu, memory, data_addr);

			break;
		//
		// BEQ
		//
		case OPCODE_BEQ_REL:
			#if DEBUG_OPCODES
				printf("BEQ_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BEQ(cpu, memory, data_addr);

			break;
		//
		// BIT
		//
		case OPCODE_BIT_ABS:
			#if DEBUG_OPCODES
				printf("BIT_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_ABS_IIX:
			#if DEBUG_OPCODES
				printf("BIT_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_DIR:
			#if DEBUG_OPCODES
				printf("BIT_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_DIR_IX:
			#if DEBUG_OPCODES
				printf("BIT_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			BIT(cpu, memory, data_addr);

			break;
		case OPCODE_BIT_IMM:
			#if DEBUG_OPCODES
				printf("BIT_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			BIT_IMM(cpu, memory, data_addr);

			break;
		//
		// BMI
		//
		case OPCODE_BMI_REL:
			#if DEBUG_OPCODES
				printf("BMI_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BMI(cpu, memory, data_addr);

			break;
		//
		// BNE
		//
		case OPCODE_BNE_REL:
			#if DEBUG_OPCODES
				printf("BNE_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BNE(cpu, memory, data_addr);

			break;
		//
		// BPL
		//
		case OPCODE_BPL_REL:
			#if DEBUG_OPCODES
				printf("BPL_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BPL(cpu, memory, data_addr);

			break;
		//
		// BRA
		//
		case OPCODE_BRA_REL:
			#if DEBUG_OPCODES
				printf("BRA_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BRA(cpu, memory, data_addr);

			break;
		//
		// BRK
		//
		case OPCODE_BRK_STK:
			#if DEBUG_OPCODES
				printf("BRK_STK\n");
			#endif
			BRK(cpu, memory);

			break;
		//
		// BRL
		//
		case OPCODE_BRL_REL_L:
			#if DEBUG_OPCODES
				printf("BRL_REL_L\n");
			#endif
			data_addr = addr_REL_L(cpu);
			BRL(cpu, memory, data_addr);

			break;
		//
		// BVC
		//
		case OPCODE_BVC_REL:
			#if DEBUG_OPCODES
				printf("BVC_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BVC(cpu, memory, data_addr);

			break;
		//
		// BVS
		//
		case OPCODE_BVS_REL:
			#if DEBUG_OPCODES
				printf("BVS_REL\n");
			#endif
			data_addr = addr_REL(cpu);
			BVS(cpu, memory, data_addr);

			break;
		//
		// CLC
		//
		case OPCODE_CLC_IMP:
			#if DEBUG_OPCODES
				printf("CLC_IMP\n");
			#endif
			CLC(cpu);

			break;
		//
		// CLD
		//
		case OPCODE_CLD_IMP:
			#if DEBUG_OPCODES
				printf("CLD_IMP\n");
			#endif
			CLD(cpu);

			break;
		//
		// CLI
		//
		case OPCODE_CLI_IMP:
			#if DEBUG_OPCODES
				printf("CLI_IMP\n");
			#endif
			CLI(cpu);

			break;
		//
		// CLV
		//
		case OPCODE_CLV_IMP:
			#if DEBUG_OPCODES
				printf("CLV_IMP\n");
			#endif
			CLV(cpu);

			break;
		//
		// CMP
		//
		case OPCODE_CMP_ABS:
			#if DEBUG_OPCODES
				printf("CMP_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_IIX:
			#if DEBUG_OPCODES
				printf("CMP_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_IIY:
			#if DEBUG_OPCODES
				printf("CMP_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_L:
			#if DEBUG_OPCODES
				printf("CMP_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_ABS_LIX:
			#if DEBUG_OPCODES
				printf("CMP_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR:
			#if DEBUG_OPCODES
				printf("CMP_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_STK_R:
			#if DEBUG_OPCODES
				printf("CMP_STK_R\n");
			#endif
			data_addr = addr_STK_R(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IX:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_I:
			#if DEBUG_OPCODES
				printf("CMP_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IL:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_STK_RII:
			#if DEBUG_OPCODES
				printf("CMP_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IIX:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_IIY:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_DIR_ILI:
			#if DEBUG_OPCODES
				printf("CMP_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			CMP(cpu, memory, data_addr);

			break;
		case OPCODE_CMP_IMM:
			#if DEBUG_OPCODES
				printf("CMP_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			CMP(cpu, memory, data_addr);

			break;
		//
		// COP
		//
		case OPCODE_COP_STK:
			#if DEBUG_OPCODES
				printf("COP_STK\n");
			#endif
			COP(cpu, memory);

			break;
		//
		// CPX
		//
		case OPCODE_CPX_ABS:
			#if DEBUG_OPCODES
				printf("CPX_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			CPX(cpu, memory, data_addr);

			break;
		case OPCODE_CPX_DIR:
			#if DEBUG_OPCODES
				printf("CPX_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			CPX(cpu, memory, data_addr);

			break;
		case OPCODE_CPX_IMM:
			#if DEBUG_OPCODES
				printf("CPX_IMM\n");
			#endif
			data_addr = addr_IMM_X(cpu);
			CPX(cpu, memory, data_addr);

			break;
		//
		// CPY
		//
		case OPCODE_CPY_ABS:
			#if DEBUG_OPCODES
				printf("CPY_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			CPY(cpu, memory, data_addr);

			break;
		case OPCODE_CPY_DIR:
			#if DEBUG_OPCODES
				printf("CPY_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			CPY(cpu, memory, data_addr);

			break;
		case OPCODE_CPY_IMM:
			#if DEBUG_OPCODES
				printf("CPY_IMM\n");
			#endif
			data_addr = addr_IMM_X(cpu);
			CPY(cpu, memory, data_addr);

			break;
		//
		// DEC
		//
		case OPCODE_DEC_ABS:
			#if DEBUG_OPCODES
				printf("DEC_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		case OPCODE_DEC_ACC:
			#if DEBUG_OPCODES
				printf("DEC_ACC\n");
			#endif
			DEC_A(cpu);

			break;
		case OPCODE_DEC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("DEC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		case OPCODE_DEC_DIR:
			#if DEBUG_OPCODES
				printf("DEC_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		case OPCODE_DEC_DIR_IX:
			#if DEBUG_OPCODES
				printf("DEC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			DEC(cpu, memory, data_addr);

			break;
		//
		// DEX
		//
		case OPCODE_DEX_IMP:
			#if DEBUG_OPCODES
				printf("DEX_IMP\n");
			#endif
			DEX(cpu);

			break;
		//
		// DEY
		//
		case OPCODE_DEY_IMP:
			#if DEBUG_OPCODES
				printf("DEY_IMP\n");
			#endif
			DEY(cpu);

			break;
		//
		// EOR
		//
		case OPCODE_EOR_ABS:
			#if DEBUG_OPCODES
				printf("EOR_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_IIX:
			#if DEBUG_OPCODES
				printf("EOR_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_IIY:
			#if DEBUG_OPCODES
				printf("EOR_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_L:
			#if DEBUG_OPCODES
				printf("EOR_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_ABS_LIX:
			#if DEBUG_OPCODES
				printf("EOR_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR:
			#if DEBUG_OPCODES
				printf("EOR_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_STK_R:
			#if DEBUG_OPCODES
				printf("EOR_STK_R\n");
			#endif
			data_addr = addr_STK_R(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IX:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_I:
			#if DEBUG_OPCODES
				printf("EOR_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IL:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_STK_RII:
			#if DEBUG_OPCODES
				printf("EOR_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IIX:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_IIY:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_DIR_ILI:
			#if DEBUG_OPCODES
				printf("EOR_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			EOR(cpu, memory, data_addr);

			break;
		case OPCODE_EOR_IMM:
			#if DEBUG_OPCODES
				printf("EOR_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			EOR(cpu, memory, data_addr);

			break;
		//
		// INC
		//
		case OPCODE_INC_ABS:
			#if DEBUG_OPCODES
				printf("INC_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		case OPCODE_INC_ACC:
			#if DEBUG_OPCODES
				printf("INC_ACC\n");
			#endif
			INC_A(cpu);

			break;
		case OPCODE_INC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("INC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		case OPCODE_INC_DIR:
			#if DEBUG_OPCODES
				printf("INC_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		case OPCODE_INC_DIR_IX:
			#if DEBUG_OPCODES
				printf("INC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			INC(cpu, memory, data_addr);

			break;
		//
		// INX
		//
		case OPCODE_INX_IMP:
			#if DEBUG_OPCODES
				printf("INX_IMP\n");
			#endif
			INX(cpu);
			
			break;
		//
		// INY
		//
		case OPCODE_INY_IMP:
			#if DEBUG_OPCODES
				printf("INY_IMP\n");
			#endif
			INY(cpu);
			
			break;
		//
		// JML
		//
		case OPCODE_JML_ABS_IL:
			#if DEBUG_OPCODES
				printf("JML_ABS_IL\n");
			#endif
			data_addr = addr_ABS_IL(cpu, memory);
			JMP(cpu, data_addr);

			break;
		case OPCODE_JML_ABS_L:
			#if DEBUG_OPCODES
				printf("JML_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			JMP(cpu, data_addr);

			break;
		//
		// JMP
		//
		case OPCODE_JMP_ABS:
			#if DEBUG_OPCODES
				printf("JMP_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			JMP(cpu, data_addr);

			break;
		case OPCODE_JMP_ABS_I:
			#if DEBUG_OPCODES
				printf("JMP_ABS_I\n");
			#endif
			data_addr = addr_ABS_I(cpu, memory);
			JMP(cpu, data_addr);

			break;
		case OPCODE_JMP_ABS_II:
			#if DEBUG_OPCODES
				printf("JMP_ABS_II\n");
			#endif
			data_addr = addr_ABS_II(cpu, memory);
			JMP(cpu, data_addr);

			break;
		//
		// JSL
		//
		case OPCODE_JSL_ABS_L:
			#if DEBUG_OPCODES
				printf("JSL_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			JSL(cpu, memory, data_addr);

			break;
		//
		// JSR
		//
		case OPCODE_JSR_ABS:
			#if DEBUG_OPCODES
				printf("JSR_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			JSR(cpu, memory, data_addr);

			break;
		case OPCODE_JSR_ABS_II:
			#if DEBUG_OPCODES
				printf("JSR_ABS_II\n");
			#endif
			data_addr = addr_ABS_II(cpu, memory);
			JSR(cpu, memory, data_addr);

			break;
		//
		// LDA
		//
		case OPCODE_LDA_ABS:
			#if DEBUG_OPCODES
				printf("LDA_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_IIX:
			#if DEBUG_OPCODES
				printf("LDA_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_IIY:
			#if DEBUG_OPCODES
				printf("LDA_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_L:
			#if DEBUG_OPCODES
				printf("LDA_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_ABS_LIX:
			#if DEBUG_OPCODES
				printf("LDA_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR:
			#if DEBUG_OPCODES
				printf("LDA_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_STK_R:
			#if DEBUG_OPCODES
				printf("LDA_STK_R\n");
			#endif
			data_addr = addr_STK_R(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IX:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_I:
			#if DEBUG_OPCODES
				printf("LDA_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IL:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_STK_RII:
			#if DEBUG_OPCODES
				printf("LDA_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IIX:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_IIY:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_DIR_ILI:
			#if DEBUG_OPCODES
				printf("LDA_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			LDA(cpu, memory, data_addr);

			break;
		case OPCODE_LDA_IMM:
			#if DEBUG_OPCODES
				printf("LDA_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			LDA(cpu, memory, data_addr);

			break;
		//
		// LDX
		//
		case OPCODE_LDX_ABS:
			#if DEBUG_OPCODES
				printf("LDX_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_ABS_IIY:
			#if DEBUG_OPCODES
				printf("LDX_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_DIR:
			#if DEBUG_OPCODES
				printf("LDX_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_DIR_IY:
			#if DEBUG_OPCODES
				printf("LDX_DIR_IY\n");
			#endif
			data_addr = addr_DIR_IY(cpu, memory);
			LDX(cpu, memory, data_addr);

			break;
		case OPCODE_LDX_IMM:
			#if DEBUG_OPCODES
				printf("LDX_IMM\n");
			#endif
			data_addr = addr_IMM_X(cpu);
			LDX(cpu, memory, data_addr);

			break;
		//
		// LDY
		//
		case OPCODE_LDY_ABS:
			#if DEBUG_OPCODES
				printf("LDY_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_ABS_IIX:
			#if DEBUG_OPCODES
				printf("LDY_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_DIR:
			#if DEBUG_OPCODES
				printf("LDY_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_DIR_IX:
			#if DEBUG_OPCODES
				printf("LDY_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			LDY(cpu, memory, data_addr);

			break;
		case OPCODE_LDY_IMM:
			#if DEBUG_OPCODES
				printf("LDY_IMM\n");
			#endif
			data_addr = addr_IMM_X(cpu);
			LDY(cpu, memory, data_addr);

			break;
		//
		// LSR
		//
		case OPCODE_LSR_ABS:
			#if DEBUG_OPCODES
				printf("LSR_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		case OPCODE_LSR_ACC:
			#if DEBUG_OPCODES
				printf("LSR_ACC\n");
			#endif
			LSR_A(cpu);

			break;
		case OPCODE_LSR_ABS_IIX:
			#if DEBUG_OPCODES
				printf("LSR_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		case OPCODE_LSR_DIR:
			#if DEBUG_OPCODES
				printf("LSR_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		case OPCODE_LSR_DIR_IX:
			#if DEBUG_OPCODES
				printf("LSR_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			LSR(cpu, memory, data_addr);

			break;
		//
		// MVN
		//
		case OPCODE_MVN_XYC:
			#if DEBUG_OPCODES
				printf("MVN_XYC\n");
			#endif
			data_addr = addr_XYC(cpu);
			MVN(cpu, memory, data_addr);

			break;
		//
		// MVP
		//
		case OPCODE_MVP_XYC:
			#if DEBUG_OPCODES
				printf("MVP_XYC\n");
			#endif
			data_addr = addr_XYC(cpu);
			MVP(cpu, memory, data_addr);

			break;
		//
		// NOP
		//
		case OPCODE_NOP_IMP:
			#if DEBUG_OPCODES
				printf("NOP_IMP\n");
			#endif
			NOP();

			break;
		//
		// ORA
		//
		case OPCODE_ORA_ABS:
			#if DEBUG_OPCODES
				printf("ORA_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ORA_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_IIY:
			#if DEBUG_OPCODES
				printf("ORA_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_L:
			#if DEBUG_OPCODES
				printf("ORA_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_ABS_LIX:
			#if DEBUG_OPCODES
				printf("ORA_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR:
			#if DEBUG_OPCODES
				printf("ORA_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_STK_R:
			#if DEBUG_OPCODES
				printf("ORA_STK_R\n");
			#endif
			data_addr = addr_STK_R(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IX:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_I:
			#if DEBUG_OPCODES
				printf("ORA_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IL:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_STK_RII:
			#if DEBUG_OPCODES
				printf("ORA_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IIX:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_IIY:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_DIR_ILI:
			#if DEBUG_OPCODES
				printf("ORA_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			ORA(cpu, memory, data_addr);

			break;
		case OPCODE_ORA_IMM:
			#if DEBUG_OPCODES
				printf("ORA_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			ORA(cpu, memory, data_addr);

			break;
		//
		// PEA
		//
		case OPCODE_PEA_STK:
			#if DEBUG_OPCODES
				printf("PEA_STK\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			PEA(cpu, memory, data_addr);
			
			break;
		//
		// PEI
		//
		case OPCODE_PEI_STK:
			#if DEBUG_OPCODES
				printf("PEI_STK\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			PEI(cpu, memory, data_addr);
			
			break;
		//
		// PER
		//
		case OPCODE_PER_STK:
			#if DEBUG_OPCODES
				printf("PER_STK\n");
			#endif
			data_addr = addr_REL_L(cpu);
			PER(cpu, memory, data_addr);
			
			break;
		//
		// PHA
		//
		case OPCODE_PHA_STK:
			#if DEBUG_OPCODES
				printf("PHA_STK\n");
			#endif
			PHA(cpu, memory);

			break;
		//
		// PHB
		//
		case OPCODE_PHB_STK:
			#if DEBUG_OPCODES
				printf("PHB_STK\n");
			#endif
			PHB(cpu, memory);

			break;
		//
		// PHD
		//
		case OPCODE_PHD_STK:
			#if DEBUG_OPCODES
				printf("PHD_STK\n");
			#endif
			PHD(cpu, memory);

			break;
		//
		// PHK
		//
		case OPCODE_PHK_STK:
			#if DEBUG_OPCODES
				printf("PHK_STK\n");
			#endif
			PHK(cpu, memory);

			break;
		//
		// PHP
		//
		case OPCODE_PHP_STK:
			#if DEBUG_OPCODES
				printf("PHP_STK\n");
			#endif
			PHP(cpu, memory);

			break;
		//
		// PHX
		//
		case OPCODE_PHX_STK:
			#if DEBUG_OPCODES
				printf("PHX_STK\n");
			#endif
			PHX(cpu, memory);

			break;
		//
		// PHY
		//
		case OPCODE_PHY_STK:
			#if DEBUG_OPCODES
				printf("PHY_STK\n");
			#endif
			PHY(cpu, memory);

			break;
		//
		// PLA
		//
		case OPCODE_PLA_STK:
			#if DEBUG_OPCODES
				printf("PLA_STK\n");
			#endif
			PLA(cpu, memory);

			break;
		//
		// PLB
		//
		case OPCODE_PLB_STK:
			#if DEBUG_OPCODES
				printf("PLB_STK\n");
			#endif
			PLB(cpu, memory);

			break;
		//
		// PLD
		//
		case OPCODE_PLD_STK:
			#if DEBUG_OPCODES
				printf("PLD_STK\n");
			#endif
			PLD(cpu, memory);

			break;
		//
		// PLP
		//
		case OPCODE_PLP_STK:
			#if DEBUG_OPCODES
				printf("PLP_STK\n");
			#endif
			PLP(cpu, memory);

			break;
		//
		// PLX
		//
		case OPCODE_PLX_STK:
			#if DEBUG_OPCODES
				printf("PLX_STK\n");
			#endif
			PLX(cpu, memory);

			break;
		//
		// PLY
		//
		case OPCODE_PLY_STK:
			#if DEBUG_OPCODES
				printf("PLY_STK\n");
			#endif
			PLY(cpu, memory);

			break;
		//
		// REP
		//
		case OPCODE_REP_IMM:
			#if DEBUG_OPCODES
				printf("REP_IMM\n");
			#endif
			data_addr = addr_IMM_8(cpu);
			REP(cpu, memory, data_addr);

			break;
		//
		// ROL
		//
		case OPCODE_ROL_ABS:
			#if DEBUG_OPCODES
				printf("ROL_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		case OPCODE_ROL_ACC:
			#if DEBUG_OPCODES
				printf("ROL_ACC\n");
			#endif
			ROL_A(cpu, memory);

			break;
		case OPCODE_ROL_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ROL_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		case OPCODE_ROL_DIR:
			#if DEBUG_OPCODES
				printf("ROL_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		case OPCODE_ROL_DIR_IX:
			#if DEBUG_OPCODES
				printf("ROL_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			ROL(cpu, memory, data_addr);

			break;
		//
		// ROR
		//
		case OPCODE_ROR_ABS:
			#if DEBUG_OPCODES
				printf("ROR_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		case OPCODE_ROR_ACC:
			#if DEBUG_OPCODES
				printf("ROR_ACC\n");
			#endif
			ROR_A(cpu, memory);

			break;
		case OPCODE_ROR_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ROR_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		case OPCODE_ROR_DIR:
			#if DEBUG_OPCODES
				printf("ROR_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		case OPCODE_ROR_DIR_IX:
			#if DEBUG_OPCODES
				printf("ROR_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			ROR(cpu, memory, data_addr);

			break;
		//
		// RTI
		//
		case OPCODE_RTI_STK:
			#if DEBUG_OPCODES
				printf("RTI_STK\n");
			#endif
			RTI(cpu, memory);

			break;
		//
		// RTL
		//
		case OPCODE_RTL_STK:
			#if DEBUG_OPCODES
				printf("RTL_STK\n");
			#endif
			RTL(cpu, memory);

			break;
		//
		// RTS
		//
		case OPCODE_RTS_STK:
			#if DEBUG_OPCODES
				printf("RTS_STK\n");
			#endif
			RTS(cpu, memory);

			break;
		//
		// SBC
		//
		case OPCODE_SBC_ABS:
			#if DEBUG_OPCODES
				printf("SBC_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("SBC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_ABS_IIY:
			#if DEBUG_OPCODES
				printf("SBC_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_ABS_L:
			#if DEBUG_OPCODES
				printf("SBC_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_ABS_LIX:
			#if DEBUG_OPCODES
				printf("SBC_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR:
			#if DEBUG_OPCODES
				printf("SBC_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_STK_R:
			#if DEBUG_OPCODES
				printf("SBC_STK_R\n");
			#endif
			data_addr = addr_STK_R(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IX:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_I:
			#if DEBUG_OPCODES
				printf("SBC_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IL:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_STK_RII:
			#if DEBUG_OPCODES
				printf("SBC_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IIX:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_IIY:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_DIR_ILI:
			#if DEBUG_OPCODES
				printf("SBC_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			SBC(cpu, memory, data_addr);

			break;
		case OPCODE_SBC_IMM:
			#if DEBUG_OPCODES
				printf("SBC_IMM\n");
			#endif
			data_addr = addr_IMM_M(cpu);
			SBC(cpu, memory, data_addr);

			break;
		//
		// SEC
		//
		case OPCODE_SEC_IMP:
			#if DEBUG_OPCODES
				printf("SEC_IMP\n");
			#endif
			SEC(cpu);

			break;
		//
		// SED
		//
		case OPCODE_SED_IMP:
			#if DEBUG_OPCODES
				printf("SED_IMP\n");
			#endif
			SED(cpu);

			break;
		//
		// SEI
		//
		case OPCODE_SEI_IMP:
			#if DEBUG_OPCODES
				printf("SEI_IMP\n");
			#endif
			SEI(cpu);

			break;
		//
		// SEP
		//
		case OPCODE_SEP_IMM:
			#if DEBUG_OPCODES
				printf("SEP_IMM\n");
			#endif
			data_addr = addr_IMM_8(cpu);
			SEP(cpu, memory, data_addr);

			break;
		//
		// STA
		//
		case OPCODE_STA_ABS:
			#if DEBUG_OPCODES
				printf("STA_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_ABS_IIX:
			#if DEBUG_OPCODES
				printf("STA_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_ABS_IIY:
			#if DEBUG_OPCODES
				printf("STA_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_ABS_L:
			#if DEBUG_OPCODES
				printf("STA_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_ABS_LIX:
			#if DEBUG_OPCODES
				printf("STA_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR:
			#if DEBUG_OPCODES
				printf("STA_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_STK_R:
			#if DEBUG_OPCODES
				printf("STA_STK_R\n");
			#endif
			data_addr = addr_STK_R(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IX:
			#if DEBUG_OPCODES
				printf("STA_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_I:
			#if DEBUG_OPCODES
				printf("STA_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IL:
			#if DEBUG_OPCODES
				printf("STA_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_STK_RII:
			#if DEBUG_OPCODES
				printf("STA_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IIX:
			#if DEBUG_OPCODES
				printf("STA_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_IIY:
			#if DEBUG_OPCODES
				printf("STA_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		case OPCODE_STA_DIR_ILI:
			#if DEBUG_OPCODES
				printf("STA_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(cpu, memory);
			STA(cpu, memory, data_addr);

			break;
		//
		// STP
		//
		case OPCODE_STP_IMP:
			#if DEBUG_OPCODES
				printf("STP_IMP\n");
			#endif
			STP(cpu);

			break;
		//
		// STX
		//
		case OPCODE_STX_ABS:
			#if DEBUG_OPCODES
				printf("STX_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			STX(cpu, memory, data_addr);

			break;
		case OPCODE_STX_DIR:
			#if DEBUG_OPCODES
				printf("STX_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			STX(cpu, memory, data_addr);

			break;
		case OPCODE_STX_DIR_IY:
			#if DEBUG_OPCODES
				printf("STX_DIR_IY\n");
			#endif
			data_addr = addr_DIR_IY(cpu, memory);
			STX(cpu, memory, data_addr);

			break;
		//
		// STY
		//
		case OPCODE_STY_ABS:
			#if DEBUG_OPCODES
				printf("STY_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			STY(cpu, memory, data_addr);

			break;
		case OPCODE_STY_DIR:
			#if DEBUG_OPCODES
				printf("STY_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			STY(cpu, memory, data_addr);

			break;
		case OPCODE_STY_DIR_IX:
			#if DEBUG_OPCODES
				printf("STY_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			STY(cpu, memory, data_addr);

			break;
		//
		// STZ
		//
		case OPCODE_STZ_ABS:
			#if DEBUG_OPCODES
				printf("STZ_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		case OPCODE_STZ_ABS_IIX:
			#if DEBUG_OPCODES
				printf("STZ_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		case OPCODE_STZ_DIR:
			#if DEBUG_OPCODES
				printf("STZ_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		case OPCODE_STZ_DIR_IX:
			#if DEBUG_OPCODES
				printf("STZ_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(cpu, memory);
			STZ(cpu, memory, data_addr);

			break;
		//
		// TAX
		//
		case OPCODE_TAX_IMP:
			#if DEBUG_OPCODES
				printf("TAX_IMP\n");
			#endif
			TAX(cpu);

			break;
		//
		// TAY
		//
		case OPCODE_TAY_IMP:
			#if DEBUG_OPCODES
				printf("TAY_IMP\n");
			#endif
			TAY(cpu);

			break;
		//
		// TCD
		//
		case OPCODE_TCD_IMP:
			#if DEBUG_OPCODES
				printf("TCD_IMP\n");
			#endif
			TCD(cpu);

			break;
		//
		// TCS
		//
		case OPCODE_TCS_IMP:
			#if DEBUG_OPCODES
				printf("TCS_IMP\n");
			#endif
			TCS(cpu);

			break;
		//
		// TDC
		//
		case OPCODE_TDC_IMP:
			#if DEBUG_OPCODES
				printf("TDC_IMP\n");
			#endif
			TDC(cpu);

			break;
		//
		// TRB
		//
		case OPCODE_TRB_ABS:
			#if DEBUG_OPCODES
				printf("TRB_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			TRB(cpu, memory, data_addr);

			break;
		case OPCODE_TRB_DIR:
			#if DEBUG_OPCODES
				printf("TRB_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			TRB(cpu, memory, data_addr);

			break;
		//
		// TSB
		//
		case OPCODE_TSB_ABS:
			#if DEBUG_OPCODES
				printf("TSB_ABS\n");
			#endif
			data_addr = addr_ABS(cpu, memory);
			TSB(cpu, memory, data_addr);

			break;
		case OPCODE_TSB_DIR:
			#if DEBUG_OPCODES
				printf("TSB_DIR\n");
			#endif
			data_addr = addr_DIR(cpu, memory);
			TSB(cpu, memory, data_addr);

			break;
		//
		// TSC
		//
		case OPCODE_TSC_IMP:
			#if DEBUG_OPCODES
				printf("TSC_IMP\n");
			#endif
			TSC(cpu);

			break;
		//
		// TSX
		//
		case OPCODE_TSX_IMP:
			#if DEBUG_OPCODES
				printf("TSX_IMP\n");
			#endif
			TSX(cpu);

			break;
		//
		// TXA
		//
		case OPCODE_TXA_IMP:
			#if DEBUG_OPCODES
				printf("TXA_IMP\n");
			#endif
			TXA(cpu);

			break;
		//
		// TXS
		//
		case OPCODE_TXS_IMP:
			#if DEBUG_OPCODES
				printf("TXS_IMP\n");
			#endif
			TXS(cpu);

			break;
		//
		// TXY
		//
		case OPCODE_TXY_IMP:
			#if DEBUG_OPCODES
				printf("TXY_IMP\n");
			#endif
			TXY(cpu);

			break;
		//
		// TYA
		//
		case OPCODE_TYA_IMP:
			#if DEBUG_OPCODES
				printf("TYA_IMP\n");
			#endif
			TYA(cpu);

			break;
		//
		// TYX
		//
		case OPCODE_TYX_IMP:
			#if DEBUG_OPCODES
				printf("TYX_IMP\n");
			#endif
			TYX(cpu);

			break;
		//
		// WAI
		//
		case OPCODE_WAI_IMP:
			#if DEBUG_OPCODES
				printf("WAI_IMP\n");
			#endif
			WAI(cpu);

			break;
		//
		// WDM
		//
		case OPCODE_WDM_IMM:
			#if DEBUG_OPCODES
				printf("WDM_IMM\n");
			#endif
			data_addr = addr_IMM_8(cpu);
			WDM();

			break;
		//
		// XBA
		//
		case OPCODE_XBA_IMP:
			#if DEBUG_OPCODES
				printf("XBA_IMP\n");
			#endif
			XBA(cpu);

			break;
		//
		// XCE
		//
		case OPCODE_XCE_IMP:
			#if DEBUG_OPCODES
				printf("XCE_IMP\n");
			#endif
			XCE(cpu);

			break;
		default:
			break;
	}
}

void print_cpu(struct Ricoh_5A22 *cpu)
{
	char flags[9];

	flags[0] = check_bit8(cpu->cpu_status, CPU_STATUS_N) ? 'N' : 'n';
	flags[1] = check_bit8(cpu->cpu_status, CPU_STATUS_V) ? 'V' : 'v';
	
	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		flags[2] = '1';
		flags[3] = check_bit8(cpu->cpu_status, CPU_STATUS_B) ? 'B' : 'b';
	}
	else 
	{
		flags[2] = check_bit8(cpu->cpu_status, CPU_STATUS_M) ? 'M' : 'm';
		flags[3] = check_bit8(cpu->cpu_status, CPU_STATUS_X) ? 'X' : 'x';
	}

	flags[4] = check_bit8(cpu->cpu_status, CPU_STATUS_D) ? 'D' : 'd';
	flags[5] = check_bit8(cpu->cpu_status, CPU_STATUS_I) ? 'I' : 'i';
	flags[6] = check_bit8(cpu->cpu_status, CPU_STATUS_Z) ? 'Z' : 'z';
	flags[7] = check_bit8(cpu->cpu_status, CPU_STATUS_C) ? 'C' : 'c';
	flags[8] = '\0';

	printf
		(
			"%06x A:%04x X:%04x Y:%04x S:%04x D:%04x DB:%02x %s \n", 
				LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr),
				cpu->register_A,
				cpu->register_X,
				cpu->register_Y,
				cpu->stack_ptr,
				cpu->direct_page,
				cpu->data_bank,
				flags
		);
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

	swap_cpu_status(cpu, cpu->cpu_status);

	cpu->RDY = 1;
	cpu->LPM = 0;

	cpu->NMI_line = 0;
	cpu->IRQ_line = 0;
}

