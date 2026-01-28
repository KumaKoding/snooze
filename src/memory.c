#include "memory.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void init_memory(struct Memory *memory, uint8_t ROM_type_marker)
{
	memory->ROM_type_marker = ROM_type_marker;
	memory->WRAM = malloc(WRAM_SIZE * sizeof(uint8_t));
	memory->REG = malloc(REG_SIZE * sizeof(uint8_t));

	if(ROM_type_marker == LoROM_MARKER)
	{
		memory->ROM.LoROM.ROM = malloc(LoROM_ROM_SIZE * sizeof(uint8_t));
		memory->ROM.LoROM.SRAM = malloc(LoROM_SRAM_SIZE * sizeof(uint8_t));
	}
	else if(ROM_type_marker == HiROM_MARKER)
{
		memory->ROM.HiROM.ROM = malloc(HiROM_ROM_SIZE * sizeof(uint8_t));
		memory->ROM.HiROM.SRAM = malloc(HiROM_SRAM_SIZE * sizeof(uint8_t));
	}
	else if(ROM_type_marker == ExHiROM_MARKER)
	{
		memory->ROM.ExHiROM.ROM = malloc(ExHiROM_ROM_SIZE * sizeof(uint8_t));
		memory->ROM.ExHiROM.ExROM = malloc(ExHiROM_ExROM_SIZE * sizeof(uint8_t));
		memory->ROM.ExHiROM.SRAM = malloc(ExHiROM_SRAM_SIZE * sizeof(uint8_t));
	}
}

int is_within_area(uint32_t index, uint8_t bank_0, uint16_t bytes_0, uint8_t bank_1, uint16_t bytes_1)
{
	// printf("%06x %02x %04x %02x %04x\n", index, bank_0, bytes_0, bank_1, bytes_1);

	if(((index & 0x00FF0000) >> 16) < bank_0)
	{
		return 0;
	}
	
	if((index & 0x0000FFFF) < bytes_0)
	{
		return 0;
	}

	if(((index & 0x00FF0000) >> 16) > bank_1)
	{
		return 0;
	}	
	
	if((index & 0x0000FFFF) > bytes_1)
	{
		return 0;
	}

	return 1;
}

uint32_t WRAM_lowRAM_mirror_indexer(uint32_t index)
{
	uint32_t new_index = 0;	
	uint16_t low_bytes = index & 0x0000FFFF;

	new_index = (low_bytes - WRAM_BYTES[0]);

	return new_index;
}

uint32_t WRAM_indexer(uint32_t index)
{
	uint32_t new_index = 0;	
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	uint32_t bank_width = (WRAM_BYTES[1] + 1) - WRAM_BYTES[0];

	uint8_t bank_offset = bank_byte - WRAM_BANKS[0];
	uint16_t byte_offset = low_bytes - WRAM_BYTES[0];

	new_index = (bank_offset * bank_width) + byte_offset;

	return new_index;
}

uint32_t REG_indexer(uint32_t index)
{
	uint32_t new_index = 0;	
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	uint32_t bank_width = (REG_BYTES[1] + 1) - REG_BYTES[0];

	uint8_t bank_offset = bank_byte - REG_BANKS[0];
	uint16_t byte_offset = low_bytes - REG_BYTES[0];

	new_index = (bank_offset * bank_width) + byte_offset;
	
	return new_index;
}

uint32_t LoROM_ROM_indexer(uint32_t index)
{
	uint32_t new_index = 0;	
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	uint32_t bank_width = (LoROM_ROM_BYTES[1] + 1) - LoROM_ROM_BYTES[0];

	uint8_t bank_offset = bank_byte - LoROM_ROM_BANKS[0];
	uint16_t byte_offset = low_bytes - LoROM_ROM_BYTES[0];

	new_index = (bank_offset * bank_width) + byte_offset;

	return new_index;
}

uint32_t LoROM_ROM_mirror_indexer(uint32_t index)
{
	uint32_t new_index = 0;	
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = (index & 0x0000FFFF) + LoROM_ROM_BYTES[0];

	uint32_t bank_width = (LoROM_ROM_BYTES[1] + 1) - LoROM_ROM_BYTES[0];

	uint8_t bank_offset = bank_byte - LoROM_ROM_BANKS[0];
	uint16_t byte_offset = low_bytes - LoROM_ROM_BYTES[0];

	new_index = (bank_offset * bank_width) + byte_offset;

	return new_index;
}

uint32_t LoROM_SRAM_indexer(uint32_t index)
{
	uint32_t new_index = 0;	
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	uint32_t bank_width = (LoROM_SRAM_BYTES[1] + 1) - LoROM_SRAM_BYTES[0];

	uint8_t bank_offset = bank_byte - LoROM_SRAM_BANKS[0];
	uint16_t byte_offset = low_bytes - LoROM_SRAM_BYTES[0];

	new_index = (bank_offset * bank_width) + byte_offset;

	return new_index;
}

uint32_t LoROM_SRAM_mirror_indexer(uint32_t index)
{
	uint32_t new_index = 0;	
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	uint32_t bank_width = (LoROM_SRAM_BYTES[1] + 1) - LoROM_SRAM_BYTES[0];
	uint32_t sram_nbanks = (LoROM_SRAM_BANKS[1] + 1) - LoROM_SRAM_BANKS[0];
	uint8_t bank_mirror_offset = bank_byte - LoROM_SRAM_MIRROR_BANKS[0];

	bank_byte = LoROM_SRAM_BANKS[0] + (bank_mirror_offset % sram_nbanks);
	uint8_t bank_offset = bank_byte - LoROM_SRAM_BANKS[0];
	uint16_t byte_offset = low_bytes - LoROM_SRAM_BYTES[0];

	new_index = (bank_offset * bank_width) + byte_offset;

	return new_index;
}

void ROM_write(struct Memory *memory, uint32_t addr, uint8_t val)
{
	uint32_t cartridge_addr = addr;
	uint8_t cartridge_byte = (uint8_t)((addr & 0x00FF0000) >> 16);

	if(cartridge_byte < WRAM_BANKS[0])
	{
		cartridge_byte = cartridge_byte + 0x80;

		cartridge_addr = addr & 0x0000FFFF;
		cartridge_addr |= (0x00000000 | cartridge_byte) << 16;
	}

	if(IN_LoROM_ROM(cartridge_addr))
	{
		memory->ROM.LoROM.ROM[LoROM_ROM_indexer(cartridge_addr)] = val;
	}
	else if(IN_LoROM_ROM_MIRROR(cartridge_addr))
	{
		memory->ROM.LoROM.ROM[LoROM_ROM_mirror_indexer(cartridge_addr)] = val;
	}
}

// uint8_t NMI = 0xc2; // set high by default

uint8_t mem_read(struct Memory *memory, uint32_t addr)
{
	uint32_t cartridge_addr = addr;
	uint8_t cartridge_byte = (uint8_t)((addr & 0x00FF0000) >> 16);

	if(cartridge_byte < WRAM_BANKS[0])
	{
		cartridge_byte = cartridge_byte + 0x80;

		cartridge_addr = addr & 0x0000FFFF;
		cartridge_addr |= (0x00000000 | cartridge_byte) << 16;
	}

	uint32_t mirror_addr = (addr & 0x0000FFFF);
	uint8_t mirror_byte = (uint8_t)((addr & 0x00FF0000) >> 16) + 0x80;
	mirror_addr |= ((0x00000000 | mirror_byte) << 16);

	if(IN_WRAM(addr))
	{
		memory->data_bus = memory->WRAM[WRAM_indexer(addr)];
	}
	else if(IN_WRAM_LOWRAM_MIRROR(addr))
	{
		memory->data_bus = memory->WRAM[WRAM_lowRAM_mirror_indexer(addr)];
	}
	else if(IN_WRAM_LOWRAM_MIRROR(mirror_addr))
	{
		memory->data_bus = memory->WRAM[WRAM_lowRAM_mirror_indexer(mirror_addr)];
	}
	else if(IN_REG(addr))
	{
	}
	else if(IN_REG(mirror_addr)) 
	{
		memory->data_bus = memory->REG[REG_indexer(mirror_addr)];
	}
	else if(memory->ROM_type_marker == LoROM_MARKER)
	{
		if(IN_LoROM_ROM(cartridge_addr))
		{
			memory->data_bus = memory->ROM.LoROM.ROM[LoROM_ROM_indexer(cartridge_addr)];
		}
		else if(IN_LoROM_ROM_MIRROR(cartridge_addr))
		{
			memory->data_bus = memory->ROM.LoROM.ROM[LoROM_ROM_mirror_indexer(cartridge_addr)];
		}	
		else if(IN_LoROM_SRAM(cartridge_addr))
		{
			memory->data_bus = memory->ROM.LoROM.SRAM[LoROM_SRAM_indexer(cartridge_addr)];
		}
		else if(IN_LoROM_SRAM_MIRROR(cartridge_addr))
		{
			memory->data_bus = memory->ROM.LoROM.SRAM[LoROM_SRAM_mirror_indexer(cartridge_addr)];
		}
		else 
		{
			printf("LoROM UNKNOWN\n");
		}
	}
	else 
	{
		printf("UNKNOWN\n");
	}

	return memory->data_bus; // open bus
}

uint8_t DB_read(struct Memory *memory, uint32_t index)
{
	return mem_read(memory, index);
}

void mem_write(struct Memory *memory, uint32_t addr, uint8_t write_val)
{
	uint32_t cartridge_addr = addr;
	uint8_t cartridge_byte = (uint8_t)((addr & 0x00FF0000) >> 16);

	if(cartridge_byte < WRAM_BANKS[0])
	{
		cartridge_byte = cartridge_byte + 0x80;

		cartridge_addr = addr & 0x0000FFFF;
		cartridge_addr |= (0x00000000 | cartridge_byte) << 16;
	}

	uint32_t mirror_addr = (addr & 0x0000FFFF);
	uint8_t mirror_byte = (uint8_t)((addr & 0x00FF0000) >> 16) + 0x80;
	mirror_addr |= ((0x00000000 | mirror_byte) << 16);

	if(IN_WRAM(addr))
	{
		memory->WRAM[WRAM_indexer(addr)] = write_val;
	}
	else if(IN_WRAM_LOWRAM_MIRROR(addr))
	{
		memory->WRAM[WRAM_lowRAM_mirror_indexer(addr)] = write_val;
	}
	else if(IN_WRAM_LOWRAM_MIRROR(mirror_addr))
	{
		memory->WRAM[WRAM_lowRAM_mirror_indexer(mirror_addr)] = write_val;
	}
	else if(IN_REG(addr))
	{
		memory->REG[REG_indexer(addr)] = write_val;
	}
	else if(IN_REG(mirror_addr)) 
	{
		memory->REG[REG_indexer(mirror_addr)] = write_val;
	}
	else if(memory->ROM_type_marker == LoROM_MARKER)
	{
		if(IN_LoROM_ROM(cartridge_addr))
		{
			printf("WRITE TO ROM: %06x\n", addr);
		}
		else if(IN_LoROM_ROM_MIRROR(cartridge_addr))
		{
			printf("WRITE TO ROM: %06x\n", addr);
		}	
		else if(IN_LoROM_SRAM(cartridge_addr))
		{
			memory->ROM.LoROM.SRAM[LoROM_SRAM_indexer(cartridge_addr)] = write_val;
		}
		else if(IN_LoROM_SRAM_MIRROR(cartridge_addr))
		{
			memory->ROM.LoROM.SRAM[LoROM_SRAM_mirror_indexer(cartridge_addr)] = write_val;
		}
	}
}

void DB_write(struct Memory *memory, uint32_t index, uint8_t write_val)
{
	mem_write(memory, index, write_val);
}

