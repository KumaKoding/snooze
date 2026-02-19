#include "ricoh5A22.h"
#include "memory.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define LE_HBYTE16(u16) (uint8_t)((u16 & 0xFF00) >> 8)
#define LE_LBYTE16(u16) (uint8_t)(u16 & 0x00FF)

#define SWP_LE_LBYTE16(u16, u8) ((u16 & 0xFF00) | u8)
#define SWP_LE_HBYTE16(u16, u8) ((u16 & 0x00FF) | ((0x0000 | u8) << 8))

#define LE_COMBINE_BANK_SHORT(bank, us) (uint32_t)(((((0x00000000 | bank) << 8) | ((us & 0xFF00) >> 8)) << 8) | (us & 0x00FF))
#define LE_COMBINE_BANK_2BYTE(bank, b1, b2) (uint32_t)(((((0x00000000 | bank) << 8) | b2) << 8) | b1)
#define LE_COMBINE_2BYTE(b1, b2) (uint16_t)(((0x0000 | b2) << 8) | b1)
#define LE_COMBINE_3BYTE(b1, b2, b3) (uint32_t)(((((0x00000000 | b3) << 8) | b2) << 8) | b1)

static uint8_t check_bit8(uint8_t ps, uint8_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

static uint8_t check_bit16(uint16_t ps, uint16_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

static uint8_t check_bit32(uint32_t ps, uint32_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

void add_internal_operation(struct data_bus *data_bus)
{
	data_bus->A_Bus.cpu->queued_cyles += 6;
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

void push_SP(struct data_bus *data_bus, uint8_t write_val)
{
	DB_write(data_bus, get_SP(data_bus->A_Bus.cpu), write_val);
	decrement_SP(data_bus->A_Bus.cpu, 1);
}

uint8_t pull_SP(struct data_bus *data_bus)
{
	increment_SP(data_bus->A_Bus.cpu, 1);
	return DB_read(data_bus, get_SP(data_bus->A_Bus.cpu));
}

uint32_t addr_ABS(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(data_bus, oper), DB_read(data_bus, oper + 1));

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_IIX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;	

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//
	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(data_bus, oper), DB_read(data_bus, oper + 1));

	addr += get_X(cpu);
	if(index_size(cpu) == 8 && ((addr & 0xff00) != ((addr - get_X(cpu)) & 0xff00)))
	{
		add_internal_operation(data_bus);
	}

	cpu->program_ctr += 2;


	return addr;
}

uint32_t addr_ABS_IIY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	uint32_t addr = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(data_bus, oper), DB_read(data_bus, oper + 1));

	addr += get_Y(cpu);
	if(index_size(cpu) == 8 && ((addr & 0xff00) != ((addr - get_Y(cpu)) & 0xff00)))
	{
		add_internal_operation(data_bus);
	}

	cpu->program_ctr += 2;

	return addr;
}

uint32_t addr_ABS_L(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_3BYTE(DB_read(data_bus, oper), DB_read(data_bus, oper + 1), DB_read(data_bus, oper + 2));

	cpu->program_ctr += 3;

	return addr;
}

uint32_t addr_ABS_LIX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = LE_COMBINE_3BYTE(DB_read(data_bus, oper), DB_read(data_bus, oper + 1), DB_read(data_bus, oper + 2)) + get_X(cpu);

	cpu->program_ctr += 3;

	return addr;
}

uint32_t addr_ABS_I(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_2BYTE(DB_read(data_bus, oper), DB_read(data_bus, oper + 1));
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->program_bank, addr_indirect);

	cpu->program_ctr += 2;

	return addr_effective;
}

uint32_t addr_ABS_IL(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = LE_COMBINE_2BYTE(DB_read(data_bus, oper), DB_read(data_bus, oper + 1));
	uint32_t addr_indirect = LE_COMBINE_3BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1), DB_read(data_bus, addr + 2));

	cpu->program_ctr += 2;

	return addr_indirect;
}

uint32_t addr_ABS_II(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = LE_COMBINE_2BYTE(DB_read(data_bus, oper), DB_read(data_bus, oper + 1)) + get_X(cpu);
	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->program_bank, addr_indirect);

	cpu->program_ctr += 2;

	return addr_effective;
}

uint32_t addr_DIR(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	uint32_t addr = DB_read(data_bus, oper);

	if(cpu->direct_page > 0)
	{
		addr += cpu->direct_page;
		add_internal_operation(data_bus);
	}

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_STK_R(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = DB_read(data_bus, oper);

	addr += get_SP(cpu);
	add_internal_operation(data_bus);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = DB_read(data_bus, oper);

	if(cpu->direct_page > 0)
	{
		addr += cpu->direct_page;
		add_internal_operation(data_bus);
	}

	addr += get_X(cpu);
	add_internal_operation(data_bus);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_IY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = DB_read(data_bus, oper);

	if(cpu->direct_page > 0)
	{
		addr += cpu->direct_page;
		add_internal_operation(data_bus);
	}

	addr += get_Y(cpu);
	add_internal_operation(data_bus);

	cpu->program_ctr += 1;

	return addr;
}

uint32_t addr_DIR_I(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = DB_read(data_bus, oper);

	if(cpu->direct_page > 0)
	{
		addr += cpu->direct_page;
		add_internal_operation(data_bus);
	}

	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->data_bank, addr_indirect);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IL(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//
	uint32_t addr = DB_read(data_bus, oper);
	
	if(cpu->direct_page > 0)
	{
		addr += cpu->direct_page;
		add_internal_operation(data_bus);
	}

	uint32_t addr_effective = LE_COMBINE_3BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1), DB_read(data_bus, addr + 2));

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_STK_RII(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = DB_read(data_bus, oper);

	addr += get_SP(cpu);
	add_internal_operation(data_bus);
	
	uint32_t addr_base = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
	uint32_t addr_effective = addr_base + get_Y(cpu);
	add_internal_operation(data_bus);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IIX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = DB_read(data_bus, oper);

	if(cpu->direct_page > 0)
	{
		addr += cpu->direct_page;
		add_internal_operation(data_bus);
	}

	addr += get_X(cpu);

	uint32_t addr_indirect = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

	uint32_t addr_effective = LE_COMBINE_BANK_SHORT(cpu->data_bank, addr_indirect);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_IIY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//
	//
	uint32_t addr = DB_read(data_bus, oper);

	if(cpu->direct_page > 0)
	{
		addr += cpu->direct_page;
		add_internal_operation(data_bus);
	}

	uint32_t addr_indirect = LE_COMBINE_BANK_2BYTE(cpu->data_bank, DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

	uint32_t addr_effective = addr_indirect + get_Y(cpu);
	if(index_size(cpu) == 8 && ((addr_indirect & 0xff00) != (addr_effective & 0xff00)))
	{
		add_internal_operation(data_bus);
	}

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_DIR_ILI(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	//
	// CHECK FOR OVERFLOWS
	//

	uint32_t addr = cpu->direct_page + DB_read(data_bus, oper);
	uint32_t addr_indirect = LE_COMBINE_3BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1), DB_read(data_bus, addr + 2));
	uint32_t addr_effective = addr_indirect + get_Y(cpu);

	cpu->program_ctr += 1;

	return addr_effective;
}

uint32_t addr_REL(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 1;

	return oper;
}

uint32_t addr_REL_L(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 2;

	return oper;
}

uint32_t addr_IMM_8(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 1;

	return oper;
}

uint32_t addr_IMM_16(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 2;

	return oper;
}

uint32_t addr_IMM_M(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

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

uint32_t addr_IMM_X(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

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

uint32_t addr_XYC(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t oper = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);

	cpu->program_ctr += 2;

	return oper;
}

void ADC(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint8_t operand = DB_read(data_bus, addr);
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
			uint8_t operand = DB_read(data_bus, addr);
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

			uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
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
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
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

void AND(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void ASL(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x80));
		
		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));
		
		add_internal_operation(data_bus);

		DB_write(data_bus, addr, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(operand));
		DB_write(data_bus, addr + 1, LE_HBYTE16(operand));
	}
}

void ASL_A(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;
	
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x80));
		
		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		add_internal_operation(data_bus);

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, operand);
	}
	else 
	{
		uint16_t operand = get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		operand = operand << 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		add_internal_operation(data_bus);

		cpu->register_A = operand;
	}
}

void BCC(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_C))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void BCS(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_C))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void BEQ(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_Z))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void BIT(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit8(operand, 0x40));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_V, check_bit16(operand, 0x4000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
}

void BIT_IMM(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t test = accumulator & operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));
	}
}

void BMI(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_N))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void BNE(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_Z))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void BPL(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_N))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void BRA(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	uint16_t prebranch = cpu->program_ctr;

	cpu->program_ctr += (int8_t)operand;
	add_internal_operation(data_bus);

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
	{
		add_internal_operation(data_bus);
	}
}

void BRK(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->program_ctr++; // signature
	DB_read(data_bus, LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr));

	cpu->NMI_line = 1;

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		push_SP(data_bus, LE_HBYTE16(cpu->program_ctr));
		push_SP(data_bus, LE_LBYTE16(cpu->program_ctr));
		push_SP(data_bus, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_B;
		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(data_bus, IRQ_VECTOR_6502[0]), DB_read(data_bus, IRQ_VECTOR_6502[1]));
	}
	else 
	{
		push_SP(data_bus, cpu->program_bank);
		push_SP(data_bus, LE_HBYTE16(cpu->program_ctr));
		push_SP(data_bus, LE_LBYTE16(cpu->program_ctr));
		push_SP(data_bus, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(data_bus, BRK_VECTOR_65816[0]), DB_read(data_bus, BRK_VECTOR_65816[1]));
	}
}

void BRL(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

	cpu->program_ctr += (int16_t)operand;
	add_internal_operation(data_bus);
}

void BVC(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(!check_bit8(cpu->cpu_status, CPU_STATUS_V))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void BVS(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	if(check_bit8(cpu->cpu_status, CPU_STATUS_V))
	{
		uint16_t prebranch = cpu->program_ctr;

		cpu->program_ctr += (int8_t)operand;
		add_internal_operation(data_bus);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E) && ((prebranch & 0xff00) != (cpu->program_ctr)))
		{
			add_internal_operation(data_bus);
		}
	}
}

void CLC(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->cpu_status &= ~CPU_STATUS_C;
}

void CLD(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->cpu_status &= ~CPU_STATUS_D;
}

void CLI(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->cpu_status &= ~CPU_STATUS_I;
}

void CLV(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->cpu_status &= ~CPU_STATUS_V;
}

void CMP(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = accumulator - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (accumulator >= operand));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = accumulator - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (accumulator >= operand));
	}
}

void COP(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;
	
	cpu->program_ctr++; // signature
	DB_read(data_bus, LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr));

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		push_SP(data_bus, LE_HBYTE16(cpu->program_ctr));
		push_SP(data_bus, LE_LBYTE16(cpu->program_ctr));
		push_SP(data_bus, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_B;
		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(data_bus, COP_VECTOR_65816[0]), DB_read(data_bus, COP_VECTOR_65816[1]));
	}
	else 
	{
		push_SP(data_bus, cpu->program_bank);
		push_SP(data_bus, LE_HBYTE16(cpu->program_ctr));
		push_SP(data_bus, LE_LBYTE16(cpu->program_ctr));
		push_SP(data_bus, cpu->cpu_status);

		cpu->cpu_status |= CPU_STATUS_I;
		cpu->cpu_status &= ~CPU_STATUS_D;

		cpu->program_bank = 0;
		cpu->program_ctr = LE_COMBINE_2BYTE(DB_read(data_bus, COP_VECTOR_65816[0]), DB_read(data_bus, COP_VECTOR_65816[1]));
	}
}

void CPX(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t index = get_X(cpu);

		uint8_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t index = get_X(cpu);

		uint16_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
	}
}

void CPY(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t index = get_Y(cpu);

		uint8_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t index = get_Y(cpu);

		int16_t result = index - operand;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, (index >= operand));
	}
}

void DEC(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		
		uint8_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		
		add_internal_operation(data_bus);

		DB_write(data_bus, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

		uint16_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(result));
		DB_write(data_bus, addr + 1, LE_HBYTE16(result));
	}
}

void DEC_A(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;
	
	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);
		
		uint8_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = get_A(cpu);

		uint16_t result = operand - 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		cpu->register_A = result;
	}
}

void DEX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void DEY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void EOR(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = operand ^ accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = operand ^ accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void INC(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		
		uint8_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

		uint16_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(result));
		DB_write(data_bus, addr + 1, LE_HBYTE16(result));
	}
}

void INC_A(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);
		
		uint8_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = get_A(cpu);

		uint16_t result = operand + 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		cpu->register_A = result;
	}
}

void INX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void INY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void JMP(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->program_bank = (addr & 0x00FF0000) >> 16;
	cpu->program_ctr = addr & 0x0000FFFF;
}

void JSR(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);
	
	push_SP(data_bus, LE_HBYTE16(cpu->program_ctr - 1));
	push_SP(data_bus, LE_LBYTE16(cpu->program_ctr - 1));

	cpu->program_ctr = addr & 0x0000FFFF;
}

void JSL(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	push_SP(data_bus, cpu->program_bank);
	cpu->program_bank = (addr & 0x00FF0000) >> 16;

	add_internal_operation(data_bus);

	push_SP(data_bus, LE_HBYTE16(cpu->program_ctr - 1));
	push_SP(data_bus, LE_LBYTE16(cpu->program_ctr - 1));
	cpu->program_ctr = addr & 0x0000FFFF;
}

void LDA(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_A = operand;
	}
}

void LDX(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_X  = SWP_LE_LBYTE16(cpu->register_X, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_X = operand;
	}
}

void LDY(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(operand, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, operand);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(operand, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (operand == 0));

		cpu->register_Y = operand;
	}
}

void LSR(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x01));

		uint8_t result = operand >> 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x0001));

		uint16_t result = operand >> 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(result));
		DB_write(data_bus, addr + 1, LE_HBYTE16(result));
	}
}

void LSR_A(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x01));

		uint8_t result = operand >> 1;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		
		add_internal_operation(data_bus);

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

		add_internal_operation(data_bus);

		cpu->register_A = result;
	}
}

void MVN(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t src_bank = DB_read(data_bus, addr);
	uint8_t dst_bank = DB_read(data_bus, addr + 1);

	cpu->data_bank = dst_bank;

	uint32_t src_addr = LE_COMBINE_BANK_SHORT(src_bank, get_X(cpu));
	uint32_t dst_addr = LE_COMBINE_BANK_SHORT(cpu->data_bank, get_Y(cpu));

	if(get_A(cpu) != 0xFFFF)
	{
		uint8_t read_byte = DB_read(data_bus, src_addr);
		DB_write(data_bus, dst_addr, read_byte);

		cpu->program_ctr -= 3;

		if(index_size(cpu) == 8)
		{
			cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, (uint8_t)get_X(cpu) + 1);
			add_internal_operation(data_bus);

			cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, (uint8_t)get_Y(cpu) + 1);
			add_internal_operation(data_bus);
		}
		else 
		{	
			cpu->register_X = get_X(cpu) + 1;
			add_internal_operation(data_bus);
		
			cpu->register_Y = get_Y(cpu) + 1;
			add_internal_operation(data_bus);
		}


		cpu->register_A--;
	}
}

void MVP(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t src_bank = DB_read(data_bus, addr);
	uint8_t dst_bank = DB_read(data_bus, addr + 1);

	cpu->data_bank = dst_bank;

	uint32_t src_addr = LE_COMBINE_BANK_SHORT(src_bank, get_X(cpu));
	uint32_t dst_addr = LE_COMBINE_BANK_SHORT(cpu->data_bank, get_Y(cpu));

	if(get_A(cpu) != 0xFFFF)
	{
		uint8_t read_byte = DB_read(data_bus, src_addr);
		DB_write(data_bus, dst_addr, read_byte);

		cpu->program_ctr -= 3;

		if(index_size(cpu) == 8)
		{
			cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, (uint8_t)get_X(cpu) - 1);
			add_internal_operation(data_bus);
			
			cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, (uint8_t)get_Y(cpu) - 1);
			add_internal_operation(data_bus);
		}
		else 
		{	
			cpu->register_X = get_X(cpu) - 1;
			add_internal_operation(data_bus);

			cpu->register_Y = get_Y(cpu) - 1;
			add_internal_operation(data_bus);
		}

		cpu->register_A--;
	}
}

void NOP(struct data_bus *data_bus)
{
	add_internal_operation(data_bus);
	
	return;
}

void ORA(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t accumulator = get_A(cpu);

		uint8_t result = operand | accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t accumulator = get_A(cpu);

		uint16_t result = operand | accumulator;

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void PEA(struct data_bus *data_bus, uint32_t addr)
{
	uint8_t addr_l = (uint8_t)(addr & 0x000000FF);
	uint8_t addr_h = (uint8_t)((addr & 0x0000FF00) >> 8);

	push_SP(data_bus, addr_h);
	push_SP(data_bus, addr_l);
}

void PEI(struct data_bus *data_bus, uint32_t addr)
{
	uint8_t addr_l = (uint8_t)(addr & 0x000000FF);
	uint8_t addr_h = (uint8_t)((addr & 0x0000FF00) >> 8);

	push_SP(data_bus, addr_h);
	push_SP(data_bus, addr_l);
}

void PER(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));

	// 
	// CHECK FOR OVERFLOWS
	//

	operand += cpu->program_ctr;
	add_internal_operation(data_bus);

	push_SP(data_bus, LE_HBYTE16(operand));
	push_SP(data_bus, LE_LBYTE16(operand));
}

void PHA(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t accumulator = get_A(cpu);
		add_internal_operation(data_bus);

		push_SP(data_bus, accumulator);
	}
	else 
	{
		uint16_t accumulator = get_A(cpu);
		add_internal_operation(data_bus);

		uint8_t acc_h = LE_HBYTE16(accumulator);
		uint8_t acc_l = LE_LBYTE16(accumulator);

		push_SP(data_bus, acc_h);
		push_SP(data_bus, acc_l);
	}
}

void PHB(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t data_bank = cpu->data_bank;
	add_internal_operation(data_bus);

	push_SP(data_bus, data_bank);
}

void PHD(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t direct_h = LE_HBYTE16(cpu->direct_page);
	uint8_t direct_l = LE_LBYTE16(cpu->direct_page);
	add_internal_operation(data_bus);

	push_SP(data_bus, direct_h);
	push_SP(data_bus, direct_l);
}

void PHK(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t program_bank = cpu->program_bank;
	add_internal_operation(data_bus);

	push_SP(data_bus, program_bank);
}

void PHP(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t cpu_status = cpu->cpu_status;
	add_internal_operation(data_bus);

	push_SP(data_bus, cpu_status);
}

void PHX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		uint8_t index = get_X(cpu);
		add_internal_operation(data_bus);

		push_SP(data_bus, index);
	}
	else 
	{
		uint16_t index = get_X(cpu);
		add_internal_operation(data_bus);

		uint8_t index_h = LE_HBYTE16(index);
		uint8_t index_l = LE_LBYTE16(index);

		push_SP(data_bus, index_h);
		push_SP(data_bus, index_l);
	}
}

void PHY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		uint8_t index = get_Y(cpu);
		add_internal_operation(data_bus);

		push_SP(data_bus, index);
	}
	else 
	{
		uint16_t index = get_Y(cpu);
		add_internal_operation(data_bus);

		uint8_t index_h = LE_HBYTE16(index);
		uint8_t index_l = LE_LBYTE16(index);

		push_SP(data_bus, index_h);
		push_SP(data_bus, index_l);
	}
}

void PLA(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// probably getting stack_ptr then setting address there
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	if(accumulator_size(cpu) == 8)
	{
		uint8_t accumulator = pull_SP(data_bus);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(accumulator, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (accumulator == 0));

		cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, accumulator);
	}
	else 
	{
		uint8_t accumulator_l = pull_SP(data_bus);
		uint8_t accumulator_h = pull_SP(data_bus);

		uint16_t result = LE_COMBINE_2BYTE(accumulator_h, accumulator_l);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_A = result;
	}
}

void PLB(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// probably getting stack_ptr then setting address there
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	uint8_t bank = pull_SP(data_bus);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(bank, 0x80));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (bank == 0));

	cpu->data_bank = bank;
}

void PLD(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// probably getting stack_ptr then setting address there
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	uint8_t direct_l = pull_SP(data_bus);
	uint8_t direct_h = pull_SP(data_bus);

	uint16_t result = LE_COMBINE_2BYTE(direct_l, direct_h);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x80));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

	cpu->direct_page = result;
}

void PLP(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// probably getting stack_ptr then setting address there
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	uint8_t cpu_status = pull_SP(data_bus);

	cpu->cpu_status = cpu_status;
}

void PLX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// probably getting stack_ptr then setting address there
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	if(index_size(cpu) == 8)
	{
		uint8_t index = pull_SP(data_bus);		

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(index, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (index == 0));

		cpu->register_X = SWP_LE_LBYTE16(cpu->register_X, index);
	}
	else 
	{
		uint8_t index_l = pull_SP(data_bus);
		uint8_t index_h = pull_SP(data_bus);

		uint16_t result = LE_COMBINE_2BYTE(index_l, index_h);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_X = result;
	}
}

void PLY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// probably getting stack_ptr then setting address there
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	if(index_size(cpu) == 8)
	{
		uint8_t index = pull_SP(data_bus);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(index, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (index == 0));

		cpu->register_Y = SWP_LE_LBYTE16(cpu->register_Y, index);
	}
	else 
	{
		uint8_t index_l = pull_SP(data_bus);
		uint8_t index_h = pull_SP(data_bus);
	
		uint16_t result = LE_COMBINE_2BYTE(index_l, index_h);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		cpu->register_Y = result;
	}
}

void REP(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);
	uint8_t result = cpu->cpu_status & (~operand);

	swap_cpu_status(cpu, result);
	add_internal_operation(data_bus);
}

void ROL(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t result = operand << 1;

		BIT_SECL(result, 0x01, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x80));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t result = operand << 1;

		BIT_SECL(result, 0x0001, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x8000));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(result));
		DB_write(data_bus, addr + 1, LE_HBYTE16(result));
	}
}

void ROL_A(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t accumulator = get_A(cpu);
		uint8_t result = accumulator << 1;

		BIT_SECL(result, 0x01, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(accumulator, 0x80));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		
		add_internal_operation(data_bus);

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

		add_internal_operation(data_bus);

		cpu->register_A = result;
	}
}

void ROR(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t result = operand >> 1;

		BIT_SECL(result, 0x80, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit8(operand, 0x01));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(result, 0x80));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));
		
		add_internal_operation(data_bus);

		DB_write(data_bus, addr, result);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t result = operand >> 1;

		BIT_SECL(result, 0x8000, check_bit8(cpu->cpu_status, CPU_STATUS_C));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_C, check_bit16(operand, 0x0001));

		BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(result, 0x8000));
		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (result == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(result));
		DB_write(data_bus, addr + 1, LE_HBYTE16(result));
	}
}

void ROR_A(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->queued_cyles += 6;

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

void RTI(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// point to stack_ptr???
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		swap_cpu_status(cpu, pull_SP(data_bus));

		uint8_t pc_l = pull_SP(data_bus);
		uint8_t pc_h = pull_SP(data_bus);

		cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);
	}
	else 
	{
		swap_cpu_status(cpu, pull_SP(data_bus));

		uint8_t pc_l = pull_SP(data_bus);
		uint8_t pc_h = pull_SP(data_bus);

		cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);
		cpu->program_bank = pull_SP(data_bus);
	}
}

void RTL(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// point to stack_ptr???
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	uint8_t pc_l = pull_SP(data_bus);
	uint8_t pc_h = pull_SP(data_bus);

	cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);
	cpu->program_bank = pull_SP(data_bus);

	cpu->program_ctr++;
}

void RTS(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// point to stack_ptr???
	add_internal_operation(data_bus);
	add_internal_operation(data_bus);

	uint8_t pc_l = pull_SP(data_bus);
	uint8_t pc_h = pull_SP(data_bus);

	cpu->program_ctr = LE_COMBINE_2BYTE(pc_l, pc_h);

	cpu->program_ctr++;
}

void SBC(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		if(check_bit8(cpu->cpu_status, CPU_STATUS_D))
		{
			uint8_t operand = DB_read(data_bus, addr);
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
			uint8_t operand = DB_read(data_bus, addr);
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
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
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
			uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
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

void SEC(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->cpu_status |= CPU_STATUS_C;
}

void SED(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->cpu_status |= CPU_STATUS_D;
}

void SEI(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->cpu_status |= CPU_STATUS_I;
}

void SEP(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t operand = DB_read(data_bus, addr);

	swap_cpu_status(cpu, cpu->cpu_status | operand);
	add_internal_operation(data_bus);
}

void STA(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		DB_write(data_bus, addr, LE_LBYTE16(cpu->register_A));
	}
	else 
	{
		DB_write(data_bus, addr, LE_LBYTE16(cpu->register_A));
		DB_write(data_bus, addr + 1, LE_HBYTE16(cpu->register_A));
	}
}

void STP(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	// takes 2 cycles like normal, but the last cycle is to halt the program
	cpu->LPM = 1;
	add_internal_operation(data_bus);
}

void STX(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		DB_write(data_bus, addr, LE_LBYTE16(cpu->register_X));
	}
	else 
	{
		DB_write(data_bus, addr, LE_LBYTE16(cpu->register_X));
		DB_write(data_bus, addr + 1, LE_HBYTE16(cpu->register_X));
	}
}

void STY(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(index_size(cpu) == 8)
	{
		DB_write(data_bus, addr, LE_LBYTE16(cpu->register_Y));
	}
	else 
	{
		DB_write(data_bus, addr, LE_LBYTE16(cpu->register_Y));
		DB_write(data_bus, addr + 1, LE_HBYTE16(cpu->register_Y));
	}
}

void STZ(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		DB_write(data_bus, addr, 0x00);
	}
	else 
	{
		DB_write(data_bus, addr, 0x00);
		DB_write(data_bus, addr + 1, 0x00);
	}
}

void TAX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TAY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TCD(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->register_A, 0x8000));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->register_A == 0x0000));

	cpu->direct_page = cpu->register_A;
}

void TCS(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TDC(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(cpu->direct_page, 0x8000));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (cpu->direct_page == 0x0000));

	cpu->register_A = cpu->direct_page;
}

void TRB(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t reset = operand & (~get_A(cpu));
		uint8_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, reset);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t reset = operand & (~get_A(cpu));
		uint16_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(reset));
		DB_write(data_bus, addr + 1, LE_HBYTE16(reset));
	}
}

void TSB(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(accumulator_size(cpu) == 8)
	{
		uint8_t operand = DB_read(data_bus, addr);
		uint8_t reset = operand | get_A(cpu);
		uint8_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, reset);
	}
	else 
	{
		uint16_t operand = LE_COMBINE_2BYTE(DB_read(data_bus, addr), DB_read(data_bus, addr + 1));
		uint16_t reset = operand | get_A(cpu);
		uint16_t test = operand & get_A(cpu);

		BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (test == 0));

		add_internal_operation(data_bus);

		DB_write(data_bus, addr, LE_LBYTE16(reset));
		DB_write(data_bus, addr + 1, LE_HBYTE16(reset));
	}
}

void TSC(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->register_A = get_SP(cpu);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit16(get_SP(cpu), 0x8000));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (get_SP(cpu) == 0x0000));
}

void TSX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TXA(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TXS(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TXY(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TYA(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void TYX(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

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

void WAI(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	add_internal_operation(data_bus);

	cpu->RDY = 0;
	add_internal_operation(data_bus);
}

void WDM(struct data_bus *data_bus)
{
	add_internal_operation(data_bus);

	return;
}

void XBA(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint8_t A = LE_LBYTE16(cpu->register_A);
	uint8_t B = LE_HBYTE16(cpu->register_A);

	cpu->register_A = SWP_LE_LBYTE16(cpu->register_A, B);
	add_internal_operation(data_bus);

	cpu->register_A = SWP_LE_HBYTE16(cpu->register_A, A);
	add_internal_operation(data_bus);

	BIT_SECL(cpu->cpu_status, CPU_STATUS_N, check_bit8(LE_LBYTE16(cpu->register_A), 0x80));
	BIT_SECL(cpu->cpu_status, CPU_STATUS_Z, (LE_LBYTE16(cpu->register_A) == 0x00));
}

void XCE(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;
	
	add_internal_operation(data_bus);

	uint8_t C = cpu->cpu_status & CPU_STATUS_C;
	uint8_t E = cpu->cpu_emulation6502 & CPU_STATUS_E;

	cpu->cpu_status = (cpu->cpu_status & (~CPU_STATUS_E)) | E;
	cpu->cpu_emulation6502 = (cpu->cpu_emulation6502 & (~CPU_STATUS_C)) | C;
	
	swap_cpu_status(cpu, cpu->cpu_status);
}

void hw_nmi(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->RDY = 1;

	// I don't know
	fetch(data_bus);
	// probably getting stack_ptr
	add_internal_operation(data_bus);

	uint16_t short_addr = 0x00;

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		push_SP(data_bus, cpu->program_bank);
	}

	push_SP(data_bus, LE_HBYTE16(cpu->program_ctr));
	push_SP(data_bus, LE_LBYTE16(cpu->program_ctr));
	push_SP(data_bus, cpu->cpu_status);

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		short_addr = LE_COMBINE_2BYTE(DB_read(data_bus, NMI_VECTOR_6502[0]), DB_read(data_bus, NMI_VECTOR_6502[1]));
	}
	else 
	{
		short_addr = LE_COMBINE_2BYTE(DB_read(data_bus, NMI_VECTOR_65816[0]), DB_read(data_bus, NMI_VECTOR_65816[1]));
	}

	cpu->program_bank = 0x00;
	cpu->program_ctr = short_addr;
}

void hw_reset(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	// I don't know
	fetch(data_bus);
	// probably getting stack ptr
	add_internal_operation(data_bus);

	uint16_t short_addr = 0x00;

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		push_SP(data_bus, cpu->program_bank);
	}

	push_SP(data_bus, LE_HBYTE16(cpu->program_ctr));
	push_SP(data_bus, LE_LBYTE16(cpu->program_ctr));
	push_SP(data_bus, cpu->cpu_status);

	if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
	{
		short_addr = LE_COMBINE_2BYTE(DB_read(data_bus, NMI_VECTOR_6502[0]), DB_read(data_bus, NMI_VECTOR_6502[1]));
	}

	reset_ricoh_5a22(data_bus);

	cpu->program_bank = 0x00;
	cpu->program_ctr = short_addr;
}

void hw_irq(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->RDY = 1;
	
	if(!check_bit8(cpu->cpu_status, CPU_STATUS_I))
	{
		// I don't know
		fetch(data_bus);
		// probably getting stack_ptr
		add_internal_operation(data_bus);

		uint16_t short_addr = 0x00;

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
		{
			push_SP(data_bus, cpu->program_bank);
		}

		push_SP(data_bus, LE_HBYTE16(cpu->program_ctr));
		push_SP(data_bus, LE_LBYTE16(cpu->program_ctr));
		push_SP(data_bus, cpu->cpu_status);

		if(check_bit8(cpu->cpu_emulation6502, CPU_STATUS_E))
		{
			short_addr = LE_COMBINE_2BYTE(DB_read(data_bus, NMI_VECTOR_6502[0]), DB_read(data_bus, NMI_VECTOR_6502[1]));
		}

		cpu->program_bank = 0x00;
		cpu->program_ctr = short_addr;
	}
}

uint8_t fetch(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	uint32_t opcode_addr = LE_COMBINE_BANK_SHORT(cpu->program_bank, cpu->program_ctr);
	cpu->program_ctr++;

	return DB_read(data_bus, opcode_addr);
}


#define DEBUG_OPCODES 1

void execute(struct data_bus *data_bus, uint8_t instruction)
{
	switch(instruction)
	{
		uint32_t data_addr;

		//
		// ADC
		//
		case OPCODE_ADC_ABS:
			#if DEBUG_OPCODES
				printf("ADC_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ADC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_ABS_IIY:
			#if DEBUG_OPCODES
				printf("ADC_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_ABS_L:
			#if DEBUG_OPCODES
				printf("ADC_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_ABS_LIX:
			#if DEBUG_OPCODES
				printf("ADC_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_DIR:
			#if DEBUG_OPCODES
				printf("ADC_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_STK_R:
			printf("OPCODE_ADC_STK_R\n");
			data_addr = addr_STK_R(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_DIR_IX:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_DIR_I:
			#if DEBUG_OPCODES
				printf("ADC_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_DIR_IL:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_STK_RII:
			#if DEBUG_OPCODES
				printf("ADC_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_DIR_IIX:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_DIR_IIY:
			#if DEBUG_OPCODES
				printf("ADC_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_DIR_ILI:
			#if DEBUG_OPCODES
				printf("ADC_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			ADC(data_bus, data_addr);

			break;
		case OPCODE_ADC_IMM:
			#if DEBUG_OPCODES
				printf("ADC_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			ADC(data_bus, data_addr);

			break;	
		//
		// AND
		//
		case OPCODE_AND_ABS:
			#if DEBUG_OPCODES
				printf("AND_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			AND(data_bus, data_addr);

			break;	
		case OPCODE_AND_ABS_IIX:
			#if DEBUG_OPCODES
				printf("AND_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_ABS_IIY:
			#if DEBUG_OPCODES
				printf("AND_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_ABS_L:
			#if DEBUG_OPCODES
				printf("AND_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_ABS_LIX:
			#if DEBUG_OPCODES
				printf("AND_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_DIR:
			#if DEBUG_OPCODES
				printf("AND_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_STK_R:
			#if DEBUG_OPCODES
				printf("AND_STK_R\n");
			#endif
			data_addr = addr_STK_R(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_DIR_IX:
			#if DEBUG_OPCODES
				printf("AND_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_DIR_I:
			#if DEBUG_OPCODES
				printf("AND_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_DIR_IL:
			#if DEBUG_OPCODES
				printf("AND_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_STK_RII:
			#if DEBUG_OPCODES
				printf("AND_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_DIR_IIX:
			#if DEBUG_OPCODES
				printf("AND_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_DIR_IIY:
			#if DEBUG_OPCODES
				printf("AND_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_DIR_ILI:
			#if DEBUG_OPCODES
				printf("AND_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			AND(data_bus, data_addr);

			break;
		case OPCODE_AND_IMM:
			#if DEBUG_OPCODES
				printf("AND_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			AND(data_bus, data_addr);

			break;
		//
		// ASL
		//
		case OPCODE_ASL_ABS:
			#if DEBUG_OPCODES
				printf("ASL_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			ASL(data_bus, data_addr);

			break;
		case OPCODE_ASL_ACC:
			#if DEBUG_OPCODES
				printf("ASL_ACC\n");
			#endif
			ASL_A(data_bus);

			break;
		case OPCODE_ASL_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ASL_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			ASL(data_bus, data_addr);

			break;
		case OPCODE_ASL_DIR:
			#if DEBUG_OPCODES
				printf("ASL_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			ASL(data_bus, data_addr);

			break;
		case OPCODE_ASL_DIR_IX:
			#if DEBUG_OPCODES
				printf("ASL_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			ASL(data_bus, data_addr);

			break;
		//
		// BCC
		//
		case OPCODE_BCC_REL:
			#if DEBUG_OPCODES
				printf("BCC_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BCC(data_bus, data_addr);

			break;
		//
		// BCS
		//
		case OPCODE_BCS_REL:
			#if DEBUG_OPCODES
				printf("BCS_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BCS(data_bus, data_addr);

			break;
		//
		// BEQ
		//
		case OPCODE_BEQ_REL:
			#if DEBUG_OPCODES
				printf("BEQ_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BEQ(data_bus, data_addr);

			break;
		//
		// BIT
		//
		case OPCODE_BIT_ABS:
			#if DEBUG_OPCODES
				printf("BIT_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			BIT(data_bus, data_addr);

			break;
		case OPCODE_BIT_ABS_IIX:
			#if DEBUG_OPCODES
				printf("BIT_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			BIT(data_bus, data_addr);

			break;
		case OPCODE_BIT_DIR:
			#if DEBUG_OPCODES
				printf("BIT_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			BIT(data_bus, data_addr);

			break;
		case OPCODE_BIT_DIR_IX:
			#if DEBUG_OPCODES
				printf("BIT_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			BIT(data_bus, data_addr);

			break;
		case OPCODE_BIT_IMM:
			#if DEBUG_OPCODES
				printf("BIT_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			BIT_IMM(data_bus, data_addr);

			break;
		//
		// BMI
		//
		case OPCODE_BMI_REL:
			#if DEBUG_OPCODES
				printf("BMI_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BMI(data_bus, data_addr);

			break;
		//
		// BNE
		//
		case OPCODE_BNE_REL:
			#if DEBUG_OPCODES
				printf("BNE_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BNE(data_bus, data_addr);

			break;
		//
		// BPL
		//
		case OPCODE_BPL_REL:
			#if DEBUG_OPCODES
				printf("BPL_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BPL(data_bus, data_addr);

			break;
		//
		// BRA
		//
		case OPCODE_BRA_REL:
			#if DEBUG_OPCODES
				printf("BRA_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BRA(data_bus, data_addr);

			break;
		//
		// BRK
		//
		case OPCODE_BRK_STK:
			#if DEBUG_OPCODES
				printf("BRK_STK\n");
			#endif
			BRK(data_bus);

			break;
		//
		// BRL
		//
		case OPCODE_BRL_REL_L:
			#if DEBUG_OPCODES
				printf("BRL_REL_L\n");
			#endif
			data_addr = addr_REL_L(data_bus);
			BRL(data_bus, data_addr);

			break;
		//
		// BVC
		//
		case OPCODE_BVC_REL:
			#if DEBUG_OPCODES
				printf("BVC_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BVC(data_bus, data_addr);

			break;
		//
		// BVS
		//
		case OPCODE_BVS_REL:
			#if DEBUG_OPCODES
				printf("BVS_REL\n");
			#endif
			data_addr = addr_REL(data_bus);
			BVS(data_bus, data_addr);

			break;
		//
		// CLC
		//
		case OPCODE_CLC_IMP:
			#if DEBUG_OPCODES
				printf("CLC_IMP\n");
			#endif
			CLC(data_bus);

			break;
		//
		// CLD
		//
		case OPCODE_CLD_IMP:
			#if DEBUG_OPCODES
				printf("CLD_IMP\n");
			#endif
			CLD(data_bus);

			break;
		//
		// CLI
		//
		case OPCODE_CLI_IMP:
			#if DEBUG_OPCODES
				printf("CLI_IMP\n");
			#endif
			CLI(data_bus);

			break;
		//
		// CLV
		//
		case OPCODE_CLV_IMP:
			#if DEBUG_OPCODES
				printf("CLV_IMP\n");
			#endif
			CLV(data_bus);

			break;
		//
		// CMP
		//
		case OPCODE_CMP_ABS:
			#if DEBUG_OPCODES
				printf("CMP_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_ABS_IIX:
			#if DEBUG_OPCODES
				printf("CMP_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_ABS_IIY:
			#if DEBUG_OPCODES
				printf("CMP_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_ABS_L:
			#if DEBUG_OPCODES
				printf("CMP_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_ABS_LIX:
			#if DEBUG_OPCODES
				printf("CMP_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_DIR:
			#if DEBUG_OPCODES
				printf("CMP_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_STK_R:
			#if DEBUG_OPCODES
				printf("CMP_STK_R\n");
			#endif
			data_addr = addr_STK_R(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_DIR_IX:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_DIR_I:
			#if DEBUG_OPCODES
				printf("CMP_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_DIR_IL:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_STK_RII:
			#if DEBUG_OPCODES
				printf("CMP_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_DIR_IIX:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_DIR_IIY:
			#if DEBUG_OPCODES
				printf("CMP_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_DIR_ILI:
			#if DEBUG_OPCODES
				printf("CMP_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			CMP(data_bus, data_addr);

			break;
		case OPCODE_CMP_IMM:
			#if DEBUG_OPCODES
				printf("CMP_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			CMP(data_bus, data_addr);

			break;
		//
		// COP
		//
		case OPCODE_COP_STK:
			#if DEBUG_OPCODES
				printf("COP_STK\n");
			#endif
			COP(data_bus);

			break;
		//
		// CPX
		//
		case OPCODE_CPX_ABS:
			#if DEBUG_OPCODES
				printf("CPX_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			CPX(data_bus, data_addr);

			break;
		case OPCODE_CPX_DIR:
			#if DEBUG_OPCODES
				printf("CPX_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			CPX(data_bus, data_addr);

			break;
		case OPCODE_CPX_IMM:
			#if DEBUG_OPCODES
				printf("CPX_IMM\n");
			#endif
			data_addr = addr_IMM_X(data_bus);
			CPX(data_bus, data_addr);

			break;
		//
		// CPY
		//
		case OPCODE_CPY_ABS:
			#if DEBUG_OPCODES
				printf("CPY_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			CPY(data_bus, data_addr);

			break;
		case OPCODE_CPY_DIR:
			#if DEBUG_OPCODES
				printf("CPY_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			CPY(data_bus, data_addr);

			break;
		case OPCODE_CPY_IMM:
			#if DEBUG_OPCODES
				printf("CPY_IMM\n");
			#endif
			data_addr = addr_IMM_X(data_bus);
			CPY(data_bus, data_addr);

			break;
		//
		// DEC
		//
		case OPCODE_DEC_ABS:
			#if DEBUG_OPCODES
				printf("DEC_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			DEC(data_bus, data_addr);

			break;
		case OPCODE_DEC_ACC:
			#if DEBUG_OPCODES
				printf("DEC_ACC\n");
			#endif
			DEC_A(data_bus);

			break;
		case OPCODE_DEC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("DEC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			DEC(data_bus, data_addr);

			break;
		case OPCODE_DEC_DIR:
			#if DEBUG_OPCODES
				printf("DEC_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			DEC(data_bus, data_addr);

			break;
		case OPCODE_DEC_DIR_IX:
			#if DEBUG_OPCODES
				printf("DEC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			DEC(data_bus, data_addr);

			break;
		//
		// DEX
		//
		case OPCODE_DEX_IMP:
			#if DEBUG_OPCODES
				printf("DEX_IMP\n");
			#endif
			DEX(data_bus);

			break;
		//
		// DEY
		//
		case OPCODE_DEY_IMP:
			#if DEBUG_OPCODES
				printf("DEY_IMP\n");
			#endif
			DEY(data_bus);

			break;
		//
		// EOR
		//
		case OPCODE_EOR_ABS:
			#if DEBUG_OPCODES
				printf("EOR_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_ABS_IIX:
			#if DEBUG_OPCODES
				printf("EOR_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_ABS_IIY:
			#if DEBUG_OPCODES
				printf("EOR_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_ABS_L:
			#if DEBUG_OPCODES
				printf("EOR_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_ABS_LIX:
			#if DEBUG_OPCODES
				printf("EOR_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_DIR:
			#if DEBUG_OPCODES
				printf("EOR_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_STK_R:
			#if DEBUG_OPCODES
				printf("EOR_STK_R\n");
			#endif
			data_addr = addr_STK_R(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_DIR_IX:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_DIR_I:
			#if DEBUG_OPCODES
				printf("EOR_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_DIR_IL:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_STK_RII:
			#if DEBUG_OPCODES
				printf("EOR_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_DIR_IIX:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_DIR_IIY:
			#if DEBUG_OPCODES
				printf("EOR_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_DIR_ILI:
			#if DEBUG_OPCODES
				printf("EOR_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			EOR(data_bus, data_addr);

			break;
		case OPCODE_EOR_IMM:
			#if DEBUG_OPCODES
				printf("EOR_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			EOR(data_bus, data_addr);

			break;
		//
		// INC
		//
		case OPCODE_INC_ABS:
			#if DEBUG_OPCODES
				printf("INC_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			INC(data_bus, data_addr);

			break;
		case OPCODE_INC_ACC:
			#if DEBUG_OPCODES
				printf("INC_ACC\n");
			#endif
			INC_A(data_bus);

			break;
		case OPCODE_INC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("INC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			INC(data_bus, data_addr);

			break;
		case OPCODE_INC_DIR:
			#if DEBUG_OPCODES
				printf("INC_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			INC(data_bus, data_addr);

			break;
		case OPCODE_INC_DIR_IX:
			#if DEBUG_OPCODES
				printf("INC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			INC(data_bus, data_addr);

			break;
		//
		// INX
		//
		case OPCODE_INX_IMP:
			#if DEBUG_OPCODES
				printf("INX_IMP\n");
			#endif
			INX(data_bus);
			
			break;
		//
		// INY
		//
		case OPCODE_INY_IMP:
			#if DEBUG_OPCODES
				printf("INY_IMP\n");
			#endif
			INY(data_bus);
			
			break;
		//
		// JML
		//
		case OPCODE_JML_ABS_IL:
			#if DEBUG_OPCODES
				printf("JML_ABS_IL\n");
			#endif
			data_addr = addr_ABS_IL(data_bus);
			JMP(data_bus, data_addr);

			break;
		case OPCODE_JML_ABS_L:
			#if DEBUG_OPCODES
				printf("JML_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			JMP(data_bus, data_addr);

			break;
		//
		// JMP
		//
		case OPCODE_JMP_ABS:
			#if DEBUG_OPCODES
				printf("JMP_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			JMP(data_bus, data_addr);

			break;
		case OPCODE_JMP_ABS_I:
			#if DEBUG_OPCODES
				printf("JMP_ABS_I\n");
			#endif
			data_addr = addr_ABS_I(data_bus);
			JMP(data_bus, data_addr);

			break;
		case OPCODE_JMP_ABS_II:
			#if DEBUG_OPCODES
				printf("JMP_ABS_II\n");
			#endif
			data_addr = addr_ABS_II(data_bus);
			JMP(data_bus, data_addr);

			break;
		//
		// JSL
		//
		case OPCODE_JSL_ABS_L:
			#if DEBUG_OPCODES
				printf("JSL_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			JSL(data_bus, data_addr);

			break;
		//
		// JSR
		//
		case OPCODE_JSR_ABS:
			#if DEBUG_OPCODES
				printf("JSR_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			JSR(data_bus, data_addr);

			break;
		case OPCODE_JSR_ABS_II:
			#if DEBUG_OPCODES
				printf("JSR_ABS_II\n");
			#endif
			data_addr = addr_ABS_II(data_bus);
			JSR(data_bus, data_addr);

			break;
		//
		// LDA
		//
		case OPCODE_LDA_ABS:
			#if DEBUG_OPCODES
				printf("LDA_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_ABS_IIX:
			#if DEBUG_OPCODES
				printf("LDA_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_ABS_IIY:
			#if DEBUG_OPCODES
				printf("LDA_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_ABS_L:
			#if DEBUG_OPCODES
				printf("LDA_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_ABS_LIX:
			#if DEBUG_OPCODES
				printf("LDA_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_DIR:
			#if DEBUG_OPCODES
				printf("LDA_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_STK_R:
			#if DEBUG_OPCODES
				printf("LDA_STK_R\n");
			#endif
			data_addr = addr_STK_R(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_DIR_IX:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_DIR_I:
			#if DEBUG_OPCODES
				printf("LDA_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_DIR_IL:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_STK_RII:
			#if DEBUG_OPCODES
				printf("LDA_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_DIR_IIX:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_DIR_IIY:
			#if DEBUG_OPCODES
				printf("LDA_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_DIR_ILI:
			#if DEBUG_OPCODES
				printf("LDA_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			LDA(data_bus, data_addr);

			break;
		case OPCODE_LDA_IMM:
			#if DEBUG_OPCODES
				printf("LDA_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			LDA(data_bus, data_addr);

			break;
		//
		// LDX
		//
		case OPCODE_LDX_ABS:
			#if DEBUG_OPCODES
				printf("LDX_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			LDX(data_bus, data_addr);

			break;
		case OPCODE_LDX_ABS_IIY:
			#if DEBUG_OPCODES
				printf("LDX_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			LDX(data_bus, data_addr);

			break;
		case OPCODE_LDX_DIR:
			#if DEBUG_OPCODES
				printf("LDX_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			LDX(data_bus, data_addr);

			break;
		case OPCODE_LDX_DIR_IY:
			#if DEBUG_OPCODES
				printf("LDX_DIR_IY\n");
			#endif
			data_addr = addr_DIR_IY(data_bus);
			LDX(data_bus, data_addr);

			break;
		case OPCODE_LDX_IMM:
			#if DEBUG_OPCODES
				printf("LDX_IMM\n");
			#endif
			data_addr = addr_IMM_X(data_bus);
			LDX(data_bus, data_addr);

			break;
		//
		// LDY
		//
		case OPCODE_LDY_ABS:
			#if DEBUG_OPCODES
				printf("LDY_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			LDY(data_bus, data_addr);

			break;
		case OPCODE_LDY_ABS_IIX:
			#if DEBUG_OPCODES
				printf("LDY_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			LDY(data_bus, data_addr);

			break;
		case OPCODE_LDY_DIR:
			#if DEBUG_OPCODES
				printf("LDY_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			LDY(data_bus, data_addr);

			break;
		case OPCODE_LDY_DIR_IX:
			#if DEBUG_OPCODES
				printf("LDY_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			LDY(data_bus, data_addr);

			break;
		case OPCODE_LDY_IMM:
			#if DEBUG_OPCODES
				printf("LDY_IMM\n");
			#endif
			data_addr = addr_IMM_X(data_bus);
			LDY(data_bus, data_addr);

			break;
		//
		// LSR
		//
		case OPCODE_LSR_ABS:
			#if DEBUG_OPCODES
				printf("LSR_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			LSR(data_bus, data_addr);

			break;
		case OPCODE_LSR_ACC:
			#if DEBUG_OPCODES
				printf("LSR_ACC\n");
			#endif
			LSR_A(data_bus);

			break;
		case OPCODE_LSR_ABS_IIX:
			#if DEBUG_OPCODES
				printf("LSR_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			LSR(data_bus, data_addr);

			break;
		case OPCODE_LSR_DIR:
			#if DEBUG_OPCODES
				printf("LSR_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			LSR(data_bus, data_addr);

			break;
		case OPCODE_LSR_DIR_IX:
			#if DEBUG_OPCODES
				printf("LSR_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			LSR(data_bus, data_addr);

			break;
		//
		// MVN
		//
		case OPCODE_MVN_XYC:
			#if DEBUG_OPCODES
				printf("MVN_XYC\n");
			#endif
			data_addr = addr_XYC(data_bus);
			MVN(data_bus, data_addr);

			break;
		//
		// MVP
		//
		case OPCODE_MVP_XYC:
			#if DEBUG_OPCODES
				printf("MVP_XYC\n");
			#endif
			data_addr = addr_XYC(data_bus);
			MVP(data_bus, data_addr);

			break;
		//
		// NOP
		//
		case OPCODE_NOP_IMP:
			#if DEBUG_OPCODES
				printf("NOP_IMP\n");
			#endif
			NOP(data_bus);

			break;
		//
		// ORA
		//
		case OPCODE_ORA_ABS:
			#if DEBUG_OPCODES
				printf("ORA_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ORA_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_ABS_IIY:
			#if DEBUG_OPCODES
				printf("ORA_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_ABS_L:
			#if DEBUG_OPCODES
				printf("ORA_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_ABS_LIX:
			#if DEBUG_OPCODES
				printf("ORA_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_DIR:
			#if DEBUG_OPCODES
				printf("ORA_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_STK_R:
			#if DEBUG_OPCODES
				printf("ORA_STK_R\n");
			#endif
			data_addr = addr_STK_R(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_DIR_IX:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_DIR_I:
			#if DEBUG_OPCODES
				printf("ORA_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_DIR_IL:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_STK_RII:
			#if DEBUG_OPCODES
				printf("ORA_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_DIR_IIX:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_DIR_IIY:
			#if DEBUG_OPCODES
				printf("ORA_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_DIR_ILI:
			#if DEBUG_OPCODES
				printf("ORA_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			ORA(data_bus, data_addr);

			break;
		case OPCODE_ORA_IMM:
			#if DEBUG_OPCODES
				printf("ORA_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			ORA(data_bus, data_addr);

			break;
		//
		// PEA
		//
		case OPCODE_PEA_STK:
			#if DEBUG_OPCODES
				printf("PEA_STK\n");
			#endif
			data_addr = addr_ABS(data_bus);
			PEA(data_bus, data_addr);
			
			break;
		//
		// PEI
		//
		case OPCODE_PEI_STK:
			#if DEBUG_OPCODES
				printf("PEI_STK\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			PEI(data_bus, data_addr);
			
			break;
		//
		// PER
		//
		case OPCODE_PER_STK:
			#if DEBUG_OPCODES
				printf("PER_STK\n");
			#endif
			data_addr = addr_REL_L(data_bus);
			PER(data_bus, data_addr);
			
			break;
		//
		// PHA
		//
		case OPCODE_PHA_STK:
			#if DEBUG_OPCODES
				printf("PHA_STK\n");
			#endif
			PHA(data_bus);

			break;
		//
		// PHB
		//
		case OPCODE_PHB_STK:
			#if DEBUG_OPCODES
				printf("PHB_STK\n");
			#endif
			PHB(data_bus);

			break;
		//
		// PHD
		//
		case OPCODE_PHD_STK:
			#if DEBUG_OPCODES
				printf("PHD_STK\n");
			#endif
			PHD(data_bus);

			break;
		//
		// PHK
		//
		case OPCODE_PHK_STK:
			#if DEBUG_OPCODES
				printf("PHK_STK\n");
			#endif
			PHK(data_bus);

			break;
		//
		// PHP
		//
		case OPCODE_PHP_STK:
			#if DEBUG_OPCODES
				printf("PHP_STK\n");
			#endif
			PHP(data_bus);

			break;
		//
		// PHX
		//
		case OPCODE_PHX_STK:
			#if DEBUG_OPCODES
				printf("PHX_STK\n");
			#endif
			PHX(data_bus);

			break;
		//
		// PHY
		//
		case OPCODE_PHY_STK:
			#if DEBUG_OPCODES
				printf("PHY_STK\n");
			#endif
			PHY(data_bus);

			break;
		//
		// PLA
		//
		case OPCODE_PLA_STK:
			#if DEBUG_OPCODES
				printf("PLA_STK\n");
			#endif
			PLA(data_bus);

			break;
		//
		// PLB
		//
		case OPCODE_PLB_STK:
			#if DEBUG_OPCODES
				printf("PLB_STK\n");
			#endif
			PLB(data_bus);

			break;
		//
		// PLD
		//
		case OPCODE_PLD_STK:
			#if DEBUG_OPCODES
				printf("PLD_STK\n");
			#endif
			PLD(data_bus);

			break;
		//
		// PLP
		//
		case OPCODE_PLP_STK:
			#if DEBUG_OPCODES
				printf("PLP_STK\n");
			#endif
			PLP(data_bus);

			break;
		//
		// PLX
		//
		case OPCODE_PLX_STK:
			#if DEBUG_OPCODES
				printf("PLX_STK\n");
			#endif
			PLX(data_bus);

			break;
		//
		// PLY
		//
		case OPCODE_PLY_STK:
			#if DEBUG_OPCODES
				printf("PLY_STK\n");
			#endif
			PLY(data_bus);

			break;
		//
		// REP
		//
		case OPCODE_REP_IMM:
			#if DEBUG_OPCODES
				printf("REP_IMM\n");
			#endif
			data_addr = addr_IMM_8(data_bus);
			REP(data_bus, data_addr);

			break;
		//
		// ROL
		//
		case OPCODE_ROL_ABS:
			#if DEBUG_OPCODES
				printf("ROL_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			ROL(data_bus, data_addr);

			break;
		case OPCODE_ROL_ACC:
			#if DEBUG_OPCODES
				printf("ROL_ACC\n");
			#endif
			ROL_A(data_bus);

			break;
		case OPCODE_ROL_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ROL_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			ROL(data_bus, data_addr);

			break;
		case OPCODE_ROL_DIR:
			#if DEBUG_OPCODES
				printf("ROL_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			ROL(data_bus, data_addr);

			break;
		case OPCODE_ROL_DIR_IX:
			#if DEBUG_OPCODES
				printf("ROL_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			ROL(data_bus, data_addr);

			break;
		//
		// ROR
		//
		case OPCODE_ROR_ABS:
			#if DEBUG_OPCODES
				printf("ROR_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			ROR(data_bus, data_addr);

			break;
		case OPCODE_ROR_ACC:
			#if DEBUG_OPCODES
				printf("ROR_ACC\n");
			#endif
			ROR_A(data_bus);

			break;
		case OPCODE_ROR_ABS_IIX:
			#if DEBUG_OPCODES
				printf("ROR_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			ROR(data_bus, data_addr);

			break;
		case OPCODE_ROR_DIR:
			#if DEBUG_OPCODES
				printf("ROR_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			ROR(data_bus, data_addr);

			break;
		case OPCODE_ROR_DIR_IX:
			#if DEBUG_OPCODES
				printf("ROR_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			ROR(data_bus, data_addr);

			break;
		//
		// RTI
		//
		case OPCODE_RTI_STK:
			#if DEBUG_OPCODES
				printf("RTI_STK\n");
			#endif
			RTI(data_bus);

			break;
		//
		// RTL
		//
		case OPCODE_RTL_STK:
			#if DEBUG_OPCODES
				printf("RTL_STK\n");
			#endif
			RTL(data_bus);

			break;
		//
		// RTS
		//
		case OPCODE_RTS_STK:
			#if DEBUG_OPCODES
				printf("RTS_STK\n");
			#endif
			RTS(data_bus);

			break;
		//
		// SBC
		//
		case OPCODE_SBC_ABS:
			#if DEBUG_OPCODES
				printf("SBC_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_ABS_IIX:
			#if DEBUG_OPCODES
				printf("SBC_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_ABS_IIY:
			#if DEBUG_OPCODES
				printf("SBC_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_ABS_L:
			#if DEBUG_OPCODES
				printf("SBC_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_ABS_LIX:
			#if DEBUG_OPCODES
				printf("SBC_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_DIR:
			#if DEBUG_OPCODES
				printf("SBC_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_STK_R:
			#if DEBUG_OPCODES
				printf("SBC_STK_R\n");
			#endif
			data_addr = addr_STK_R(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_DIR_IX:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_DIR_I:
			#if DEBUG_OPCODES
				printf("SBC_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_DIR_IL:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_STK_RII:
			#if DEBUG_OPCODES
				printf("SBC_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_DIR_IIX:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_DIR_IIY:
			#if DEBUG_OPCODES
				printf("SBC_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_DIR_ILI:
			#if DEBUG_OPCODES
				printf("SBC_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			SBC(data_bus, data_addr);

			break;
		case OPCODE_SBC_IMM:
			#if DEBUG_OPCODES
				printf("SBC_IMM\n");
			#endif
			data_addr = addr_IMM_M(data_bus);
			SBC(data_bus, data_addr);

			break;
		//
		// SEC
		//
		case OPCODE_SEC_IMP:
			#if DEBUG_OPCODES
				printf("SEC_IMP\n");
			#endif
			SEC(data_bus);

			break;
		//
		// SED
		//
		case OPCODE_SED_IMP:
			#if DEBUG_OPCODES
				printf("SED_IMP\n");
			#endif
			SED(data_bus);

			break;
		//
		// SEI
		//
		case OPCODE_SEI_IMP:
			#if DEBUG_OPCODES
				printf("SEI_IMP\n");
			#endif
			SEI(data_bus);

			break;
		//
		// SEP
		//
		case OPCODE_SEP_IMM:
			#if DEBUG_OPCODES
				printf("SEP_IMM\n");
			#endif
			data_addr = addr_IMM_8(data_bus);
			SEP(data_bus, data_addr);

			break;
		//
		// STA
		//
		case OPCODE_STA_ABS:
			#if DEBUG_OPCODES
				printf("STA_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_ABS_IIX:
			#if DEBUG_OPCODES
				printf("STA_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_ABS_IIY:
			#if DEBUG_OPCODES
				printf("STA_ABS_IIY\n");
			#endif
			data_addr = addr_ABS_IIY(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_ABS_L:
			#if DEBUG_OPCODES
				printf("STA_ABS_L\n");
			#endif
			data_addr = addr_ABS_L(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_ABS_LIX:
			#if DEBUG_OPCODES
				printf("STA_ABS_LIX\n");
			#endif
			data_addr = addr_ABS_LIX(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_DIR:
			#if DEBUG_OPCODES
				printf("STA_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_STK_R:
			#if DEBUG_OPCODES
				printf("STA_STK_R\n");
			#endif
			data_addr = addr_STK_R(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_DIR_IX:
			#if DEBUG_OPCODES
				printf("STA_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_DIR_I:
			#if DEBUG_OPCODES
				printf("STA_DIR_I\n");
			#endif
			data_addr = addr_DIR_I(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_DIR_IL:
			#if DEBUG_OPCODES
				printf("STA_DIR_IL\n");
			#endif
			data_addr = addr_DIR_IL(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_STK_RII:
			#if DEBUG_OPCODES
				printf("STA_STK_RII\n");
			#endif
			data_addr = addr_STK_RII(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_DIR_IIX:
			#if DEBUG_OPCODES
				printf("STA_DIR_IIX\n");
			#endif
			data_addr = addr_DIR_IIX(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_DIR_IIY:
			#if DEBUG_OPCODES
				printf("STA_DIR_IIY\n");
			#endif
			data_addr = addr_DIR_IIY(data_bus);
			STA(data_bus, data_addr);

			break;
		case OPCODE_STA_DIR_ILI:
			#if DEBUG_OPCODES
				printf("STA_DIR_ILI\n");
			#endif
			data_addr = addr_DIR_ILI(data_bus);
			STA(data_bus, data_addr);

			break;
		//
		// STP
		//
		case OPCODE_STP_IMP:
			#if DEBUG_OPCODES
				printf("STP_IMP\n");
			#endif
			STP(data_bus);

			break;
		//
		// STX
		//
		case OPCODE_STX_ABS:
			#if DEBUG_OPCODES
				printf("STX_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			STX(data_bus, data_addr);

			break;
		case OPCODE_STX_DIR:
			#if DEBUG_OPCODES
				printf("STX_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			STX(data_bus, data_addr);

			break;
		case OPCODE_STX_DIR_IY:
			#if DEBUG_OPCODES
				printf("STX_DIR_IY\n");
			#endif
			data_addr = addr_DIR_IY(data_bus);
			STX(data_bus, data_addr);

			break;
		//
		// STY
		//
		case OPCODE_STY_ABS:
			#if DEBUG_OPCODES
				printf("STY_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			STY(data_bus, data_addr);

			break;
		case OPCODE_STY_DIR:
			#if DEBUG_OPCODES
				printf("STY_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			STY(data_bus, data_addr);

			break;
		case OPCODE_STY_DIR_IX:
			#if DEBUG_OPCODES
				printf("STY_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			STY(data_bus, data_addr);

			break;
		//
		// STZ
		//
		case OPCODE_STZ_ABS:
			#if DEBUG_OPCODES
				printf("STZ_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			STZ(data_bus, data_addr);

			break;
		case OPCODE_STZ_ABS_IIX:
			#if DEBUG_OPCODES
				printf("STZ_ABS_IIX\n");
			#endif
			data_addr = addr_ABS_IIX(data_bus);
			STZ(data_bus, data_addr);

			break;
		case OPCODE_STZ_DIR:
			#if DEBUG_OPCODES
				printf("STZ_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			STZ(data_bus, data_addr);

			break;
		case OPCODE_STZ_DIR_IX:
			#if DEBUG_OPCODES
				printf("STZ_DIR_IX\n");
			#endif
			data_addr = addr_DIR_IX(data_bus);
			STZ(data_bus, data_addr);

			break;
		//
		// TAX
		//
		case OPCODE_TAX_IMP:
			#if DEBUG_OPCODES
				printf("TAX_IMP\n");
			#endif
			TAX(data_bus);

			break;
		//
		// TAY
		//
		case OPCODE_TAY_IMP:
			#if DEBUG_OPCODES
				printf("TAY_IMP\n");
			#endif
			TAY(data_bus);

			break;
		//
		// TCD
		//
		case OPCODE_TCD_IMP:
			#if DEBUG_OPCODES
				printf("TCD_IMP\n");
			#endif
			TCD(data_bus);

			break;
		//
		// TCS
		//
		case OPCODE_TCS_IMP:
			#if DEBUG_OPCODES
				printf("TCS_IMP\n");
			#endif
			TCS(data_bus);

			break;
		//
		// TDC
		//
		case OPCODE_TDC_IMP:
			#if DEBUG_OPCODES
				printf("TDC_IMP\n");
			#endif
			TDC(data_bus);

			break;
		//
		// TRB
		//
		case OPCODE_TRB_ABS:
			#if DEBUG_OPCODES
				printf("TRB_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			TRB(data_bus, data_addr);

			break;
		case OPCODE_TRB_DIR:
			#if DEBUG_OPCODES
				printf("TRB_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			TRB(data_bus, data_addr);

			break;
		//
		// TSB
		//
		case OPCODE_TSB_ABS:
			#if DEBUG_OPCODES
				printf("TSB_ABS\n");
			#endif
			data_addr = addr_ABS(data_bus);
			TSB(data_bus, data_addr);

			break;
		case OPCODE_TSB_DIR:
			#if DEBUG_OPCODES
				printf("TSB_DIR\n");
			#endif
			data_addr = addr_DIR(data_bus);
			TSB(data_bus, data_addr);

			break;
		//
		// TSC
		//
		case OPCODE_TSC_IMP:
			#if DEBUG_OPCODES
				printf("TSC_IMP\n");
			#endif
			TSC(data_bus);

			break;
		//
		// TSX
		//
		case OPCODE_TSX_IMP:
			#if DEBUG_OPCODES
				printf("TSX_IMP\n");
			#endif
			TSX(data_bus);

			break;
		//
		// TXA
		//
		case OPCODE_TXA_IMP:
			#if DEBUG_OPCODES
				printf("TXA_IMP\n");
			#endif
			TXA(data_bus);

			break;
		//
		// TXS
		//
		case OPCODE_TXS_IMP:
			#if DEBUG_OPCODES
				printf("TXS_IMP\n");
			#endif
			TXS(data_bus);

			break;
		//
		// TXY
		//
		case OPCODE_TXY_IMP:
			#if DEBUG_OPCODES
				printf("TXY_IMP\n");
			#endif
			TXY(data_bus);

			break;
		//
		// TYA
		//
		case OPCODE_TYA_IMP:
			#if DEBUG_OPCODES
				printf("TYA_IMP\n");
			#endif
			TYA(data_bus);

			break;
		//
		// TYX
		//
		case OPCODE_TYX_IMP:
			#if DEBUG_OPCODES
				printf("TYX_IMP\n");
			#endif
			TYX(data_bus);

			break;
		//
		// WAI
		//
		case OPCODE_WAI_IMP:
			#if DEBUG_OPCODES
				printf("WAI_IMP\n");
			#endif
			WAI(data_bus);

			break;
		//
		// WDM
		//
		case OPCODE_WDM_IMM:
			#if DEBUG_OPCODES
				printf("WDM_IMM\n");
			#endif
			WDM(data_bus);

			break;
		//
		// XBA
		//
		case OPCODE_XBA_IMP:
			#if DEBUG_OPCODES
				printf("XBA_IMP\n");
			#endif
			XBA(data_bus);

			break;
		//
		// XCE
		//
		case OPCODE_XCE_IMP:
			#if DEBUG_OPCODES
				printf("XCE_IMP\n");
			#endif
			XCE(data_bus);

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

void reset_ricoh_5a22(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->data_bank = 0x00;
	cpu->program_bank = 0x00;
	cpu->direct_page = 0x0000;

	cpu->program_ctr = LE_COMBINE_2BYTE(mem_read(data_bus, RESET_VECTOR_6502[0]), mem_read(data_bus, RESET_VECTOR_6502[1]));

	cpu->cpu_status = 0b00000000;
	cpu->cpu_emulation6502 = 0b00000000;

	cpu->cpu_emulation6502 = cpu->cpu_emulation6502 | CPU_STATUS_E;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_M;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_I;
	cpu->cpu_status = cpu->cpu_status | CPU_STATUS_X;

	swap_cpu_status(cpu, cpu->cpu_status);

	cpu->queued_cyles = 0;

	cpu->RDY = 1;
	cpu->LPM = 0;

	cpu->NMI_line = 0;
	cpu->IRQ_line = 0;
}

