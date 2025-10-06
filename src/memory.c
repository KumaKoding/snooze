#include "memory.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint8_t data_bus_read(struct memory *memory, uint32_t addr)
{
	return 0;
}

void init_memory(struct memory *memory, uint8_t ROM_type_marker)
{
	memory->ROM_type_marker = ROM_type_marker;
	memory->WRAM = malloc(WRAM_SIZE * sizeof(uint8_t));
	memory->low_WRAM = malloc((WRAM_LOWRAM_BYTES[1] - WRAM_LOWRAM_BYTES[0]) * sizeof(uint8_t));
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
	printf("%06x %02x %04x %02x %04x\n", index, bank_0, bytes_0, bank_1, bytes_1);

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
	uint32_t new_index = 0x00000000;
	uint16_t low_bytes = index & 0x0000FFFF;

	low_bytes = low_bytes - WRAM_LOWRAM_BYTES[0];
	new_index = new_index | low_bytes;

	return new_index;
}

uint32_t WRAM_indexer(uint32_t index)
{
	uint32_t new_index = 0x00000000;
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	bank_byte = bank_byte - WRAM_BANKS[0];
	low_bytes = low_bytes - WRAM_BYTES[0];
	new_index = ((new_index | bank_byte) << 16) | low_bytes;

	return new_index;
}

uint32_t REG_indexer(uint32_t index)
{
	uint32_t new_index = 0x00000000;
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	bank_byte = bank_byte - REG_BANKS[0];
	low_bytes = low_bytes - REG_BYTES[0];
	new_index = ((new_index | bank_byte) << 16) | low_bytes;

	return new_index;
}

uint32_t REG_mirror_indexer(uint32_t index)
{
	uint32_t new_index = 0x00000000;
	uint8_t bank_byte = (index & 0x00FF0000) >> 16;
	uint16_t low_bytes = index & 0x0000FFFF;

	bank_byte = bank_byte - REG_MIRROR_BANKS[0];
	low_bytes = low_bytes - REG_MIRROR_BYTES[0];
	new_index = ((new_index | bank_byte) << 16) | low_bytes;

	return new_index;
}

uint8_t *memory_indexer(struct memory *memory, uint32_t index)
{
	if(is_within_area(index, WRAM_LOWRAM_BANK, WRAM_LOWRAM_BYTES[0], WRAM_LOWRAM_BANK, WRAM_LOWRAM_BYTES[1]))
	{
		printf("%u\n", WRAM_indexer(index));
	}
	else if(is_within_area(index, WRAM_LOWRAM_MIRROR_1_BANKS[0], WRAM_LOWRAM_BYTES[0], WRAM_LOWRAM_MIRROR_1_BANKS[1], WRAM_LOWRAM_BYTES[1]))
	{
		printf("%u\n", WRAM_lowRAM_mirror_indexer(index));
	}
	else if(is_within_area(index, WRAM_LOWRAM_MIRROR_2_BANKS[0], WRAM_LOWRAM_BYTES[0], WRAM_LOWRAM_MIRROR_2_BANKS[1], WRAM_LOWRAM_BYTES[1]))
	{
		printf("%u\n", WRAM_lowRAM_mirror_indexer(index));
	}
	else if(is_within_area(index, WRAM_BANKS[0], WRAM_BYTES[0], WRAM_BANKS[1], WRAM_BYTES[1]))
	{
		printf("%u\n", WRAM_indexer(index));
	}
	else if(is_within_area(index, REG_BANKS[0], REG_BYTES[0], REG_BANKS[1], REG_BYTES[1]))
	{
		printf("%u\n", REG_indexer(index));
	}
	else if(is_within_area(index, REG_MIRROR_BANKS[0], REG_MIRROR_BYTES[0], REG_MIRROR_BANKS[1], REG_MIRROR_BYTES[1]))
	{
		printf("%u\n", REG_mirror_indexer(index));
	}
	else if(memory->ROM_type_marker == LoROM_MARKER)
	{
		if(is_within_area(index, LoROM_ROM_BANKS[0], LoROM_ROM_BYTES[0], LoROM_ROM_BANKS[1], LoROM_ROM_BYTES[1]))
		{
			printf("LoROM ROM\n");
		}
		else if(is_within_area(index, LoROM_MIRROR_BANKS[0], LoROM_MIRROR_BYTES[0], LoROM_MIRROR_BANKS[1], LoROM_MIRROR_BYTES[1]))
		{
			printf("LoROM mirror\n");
		}
		else if(is_within_area(index, LoROM_SRAM_BANKS[0], LoROM_SRAM_BYTES[0], LoROM_SRAM_BANKS[1], LoROM_SRAM_BYTES[1]))
		{
			printf("LoROM SRAM\n");
		}
		else 
		{
			printf("LoROM UNKNOWN\n");
		}
	}
	else if(memory->ROM_type_marker == HiROM_MARKER)
	{
		if(is_within_area(index, HiROM_ROM_BANKS[0], HiROM_ROM_BYTES[0], HiROM_ROM_BANKS[1], HiROM_ROM_BYTES[1]))
		{
			printf("HiROM ROM\n");
		}
		else if(is_within_area(index, HiROM_MIRROR_1_BANKS[0], HiROM_MIRROR_BYTES[0], HiROM_MIRROR_1_BANKS[1], HiROM_MIRROR_BYTES[1]))
		{
			printf("HiROM mirror 1\n");
		}	
		else if(is_within_area(index, HiROM_MIRROR_2_BANKS[0], HiROM_MIRROR_BYTES[0], HiROM_MIRROR_2_BANKS[1], HiROM_MIRROR_BYTES[1]))
		{
			printf("HiROM mirror 2\n");
		}	
		else if(is_within_area(index, HiROM_SRAM_BANKS[0], HiROM_SRAM_BYTES[0], HiROM_SRAM_BANKS[1], HiROM_SRAM_BYTES[1]))
		{
			printf("HiROM SRAM\n");
		}
		else 
		{
			printf("HiROM UNKNOWN\n");
		}
	}
	else if(memory->ROM_type_marker == ExHiROM_MARKER)
	{
		if(is_within_area(index, ExHiROM_ROM_BANKS[0], ExHiROM_ROM_BYTES[0], ExHiROM_ROM_BANKS[1], ExHiROM_ROM_BYTES[1]))
		{
			printf("ExHiROM ROM\n");
		}
		else if(is_within_area(index, ExHiROM_ROM_MIRROR_BANKS[0], ExHiROM_ROM_MIRROR_BYTES[0], ExHiROM_ROM_MIRROR_BANKS[1], ExHiROM_ROM_MIRROR_BYTES[1]))
		{
			printf("ExHiROM ROM mirror\n");
		}	
		else if(is_within_area(index, ExHiROM_ExROM_BANKS[0], ExHiROM_ExROM_BYTES[0], ExHiROM_ExROM_BANKS[1], ExHiROM_ExROM_BYTES[1]))
		{
			printf("ExHiROM ExROM subsection 1\n");
		}
		else if(is_within_area(index, ExHiROM_ExROM_BANKS[2], ExHiROM_ExROM_BYTES[2], ExHiROM_ExROM_BANKS[3], ExHiROM_ExROM_BYTES[3]))
		{
			printf("ExHiROM ExROM subsection 2\n");
		}
		else if(is_within_area(index, ExHiROM_ExROM_MIRROR_BANKS[0], ExHiROM_ExROM_MIRROR_BYTES[0], ExHiROM_ExROM_MIRROR_BANKS[1], ExHiROM_ExROM_MIRROR_BYTES[1]))
		{
			printf("ExHiROM ExROM mirror\n");
		}
		else if(is_within_area(index, ExHiROM_SRAM_BANKS[0], ExHiROM_SRAM_BYTES[0], ExHiROM_SRAM_BANKS[1], ExHiROM_SRAM_BYTES[1]))
		{
			printf("ExHiROM SRAM\n");
		}
		else 
		{
			printf("ExHiROM UNKNOWN\n");
		}
	}
	else 
	{
		printf("UNKNOWN\n");
	}

	return NULL;
}

