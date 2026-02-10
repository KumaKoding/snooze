#include <stdint.h>
#include "memory.h"
#include "registers.h"

void write_wram_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value)
{
	struct Memory *memory = data_bus->A_Bus.memory;

	if(addr == WMDATA)
	{
		data_bus->A_Bus.memory->WRAM[memory->WRAM_addr] = write_value;

		memory->WRAM_addr++;
	}

	if(addr == WMADDL)
	{
		memory->WRAM_addr = memory->WRAM_addr & 0xFFFFFF00;
		memory->WRAM_addr = memory->WRAM_addr | write_value;
	}

	if(addr == WMADDM)
	{
		memory->WRAM_addr = memory->WRAM_addr & 0xFFFF00FF;
		memory->WRAM_addr = memory->WRAM_addr | ((0x00000000 | write_value) << 8);
	}

	if(addr == WMADDH)
	{
		memory->WRAM_addr = memory->WRAM_addr & 0xFF00FFFF;
		memory->WRAM_addr = memory->WRAM_addr | ((0x00000000 | write_value) << 16);
	}
}

void read_wram_register(struct data_bus *data_bus, uint32_t addr)
{
	struct Memory *memory = data_bus->A_Bus.memory;

	if(addr == WMDATA)
	{
		write_register_raw(data_bus, addr, data_bus->A_Bus.memory->WRAM[memory->WRAM_addr]);

		memory->WRAM_addr++;
	}

	if(addr == WMADDL) { write_register_raw(data_bus, addr, data_bus->open_value); }
	if(addr == WMADDM) { write_register_raw(data_bus, addr, data_bus->open_value); }
	if(addr == WMADDH) { write_register_raw(data_bus, addr, data_bus->open_value); }
}

