#include "memory.h"
#include "ricoh5A22.h"
#include "PPU.h"
#include <stdint.h>
#include <stdio.h>

uint8_t DB_read(struct data_bus *data_bus, uint32_t addr)
{
	data_bus->A_Bus.cpu->queued_cyles += 8;

	return mem_read(data_bus, addr);
}

void DB_write(struct data_bus *data_bus, uint32_t index, uint8_t write_val)
{
	data_bus->A_Bus.cpu->queued_cyles += 8;

	mem_write(data_bus, index, write_val);
}

void signal_vblank(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->internal_registers.NMI_flag = 1;
	cpu->internal_registers.Vblank_flag = 1;
}

void clear_vblank(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->internal_registers.NMI_flag = 0;
	cpu->internal_registers.Vblank_flag = 0;
}

void signal_hblank(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->internal_registers.Hblank_flag = 1;
}

void clear_hblank(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	cpu->internal_registers.Hblank_flag = 0;
}

void check_IRQ(struct data_bus *data_bus)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(cpu->internal_registers.IRQEN == ENABLEHTIME)
	{
		if(ppu->x == cpu->internal_registers.horizontal_IRQ_target)
		{
			cpu->internal_registers.IRQ_flag = 1;
		}
	}

	if(cpu->internal_registers.IRQEN == ENABLEVTIME)
	{
		if(ppu->y == cpu->internal_registers.vertical_IRQ_target && ppu->x == 0)
		{
			cpu->internal_registers.IRQ_flag = 1;
		}
	}

	if(data_bus->A_Bus.cpu->internal_registers.IRQEN == ENABLEHVTIME)
	{
		if(ppu->y == cpu->internal_registers.vertical_IRQ_target && ppu->x == cpu->internal_registers.horizontal_IRQ_target)
		{
			cpu->internal_registers.IRQ_flag = 1;
		}
	}
}

