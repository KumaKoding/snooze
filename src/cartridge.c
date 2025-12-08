#include "cartridge.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

void load_ROM(const char *filename, struct Memory *memory)
{
	struct stat ROM_info;
	stat(filename, &ROM_info); 

	FILE *cartridge = fopen(filename, "rb");
	uint8_t *rom = malloc(ROM_info.st_size * sizeof(uint8_t));
	fread(rom, ROM_info.st_size, sizeof(uint8_t), cartridge);

	int byte_height = (LoROM_ROM_BYTES[1] + 1) - LoROM_ROM_BYTES[0];
	int banks = ROM_info.st_size / byte_height;

	if(ROM_info.st_size % byte_height > 0)
	{
		banks++;
	}

	for(uint8_t b = 0; b < banks; b++)
	{
		uint32_t n_bytes = b * byte_height;

		for(uint16_t addr = 0; addr < byte_height && n_bytes < ROM_info.st_size; b++)
		{
			uint32_t A_addr = ((0x00000000 | b) << 16) | addr;

			ROM_write(memory, A_addr, rom[n_bytes]);
			n_bytes++;

			printf("%u\n", n_bytes);
		}
	}

	printf("TITLE: ");

	for(int i = 0; i < 21; i++)
	{
		printf("%c", DB_read(memory, 0x00FFC0 + i));
	}

	printf("\n");

	printf("SIZE: %llu\n", ROM_info.st_size);
}
