#include "memory.h"
#include "ricoh5A22.h"
#include "PPU.h"
#include "registers.h"
#include <stdint.h>
#include <stdio.h>

#define FAST_ACCESS 6
#define MEDIUM_ACCESS 8
#define SLOW_ACCESS 12

uint8_t DB_read(struct data_bus *data_bus, uint32_t addr)
{
	uint32_t cartridge_addr = convert_to_cartridge_addr(addr);
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(IN_WRAM(cartridge_addr) || IN_WRAM_LOWRAM_MIRROR(cartridge_addr))
	{
		cpu->queued_cyles += MEDIUM_ACCESS;
	} 
	else if(IN_REG(cartridge_addr))
	{
		uint32_t reg_addr = cartridge_addr - 0x800000;

		if(0x4000 <= reg_addr && reg_addr <= 0x41FF)
		{
			cpu->queued_cyles += SLOW_ACCESS;
		}
		else
		{
			cpu->queued_cyles += FAST_ACCESS;
		}
	}
	if(data_bus->A_Bus.memory->ROM_type_marker == LoROM_MARKER)
	{
		if(IN_LoROM_ROM(cartridge_addr) || IN_LoROM_ROM_MIRROR(cartridge_addr))
		{
			if(cpu->internal_registers.fast_ROM && 0x80 <= addr && addr <= 0xFF)
			{
				cpu->queued_cyles += FAST_ACCESS;
			}
			else 
			{
				cpu->queued_cyles += MEDIUM_ACCESS;
			}
		}
		else if(IN_LoROM_SRAM(cartridge_addr) || IN_LoROM_SRAM_MIRROR(cartridge_addr))
		{
			cpu->queued_cyles += MEDIUM_ACCESS;
		}
	}

	return mem_read(data_bus, addr);
}

void DB_write(struct data_bus *data_bus, uint32_t addr, uint8_t write_val)
{
	uint32_t cartridge_addr = convert_to_cartridge_addr(addr);
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(IN_WRAM(cartridge_addr) || IN_WRAM_LOWRAM_MIRROR(cartridge_addr))
	{
		cpu->queued_cyles += MEDIUM_ACCESS;
	} 
	else if(IN_REG(cartridge_addr))
	{
		uint32_t reg_addr = cartridge_addr - 0x800000;

		if(0x4000 <= reg_addr && reg_addr <= 0x41FF)
		{
			cpu->queued_cyles += SLOW_ACCESS;
		}
		else
		{
			cpu->queued_cyles += FAST_ACCESS;
		}
	}
	if(data_bus->A_Bus.memory->ROM_type_marker == LoROM_MARKER)
	{
		if(IN_LoROM_ROM(cartridge_addr) || IN_LoROM_ROM_MIRROR(cartridge_addr))
		{
			if(cpu->internal_registers.fast_ROM && 0x80 <= addr && addr <= 0xFF)
			{
				cpu->queued_cyles += FAST_ACCESS;
			}
			else 
			{
				cpu->queued_cyles += MEDIUM_ACCESS;
			}
		}
		else if(IN_LoROM_SRAM(cartridge_addr) || IN_LoROM_SRAM_MIRROR(cartridge_addr))
		{
			cpu->queued_cyles += MEDIUM_ACCESS;
		}
	}	mem_write(data_bus, addr, write_val);
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

void set_refresh(struct data_bus *data_bus)
{
	data_bus->A_Bus.cpu->REFRESH = 1;
}

