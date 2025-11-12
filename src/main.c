#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "Ricoh5A22.h"
#include "memory.h"

int main()
{
	struct Memory memory;
	init_memory(&memory, LoROM_MARKER);

	struct stat s;
	stat("../CPU/CPUADC.sfc", &s);
	FILE *cart = fopen("../CPU/CPUADC_recompile.sfc", "rb");

	uint8_t *rom = malloc(s.st_size * sizeof(uint8_t));
	fread(rom, s.st_size, sizeof(uint8_t), cart);

	int byte_height = LoROM_ROM_BYTES[1] - LoROM_ROM_BYTES[0];

	int banks = (int)ceilf((float)s.st_size / (float)byte_height);


	printf("%d\n", banks);
	for(uint8_t i = 0; i < banks; i++)
	{
		for(uint16_t j = 0; j < byte_height && ((i * byte_height) + j) < s.st_size; j++)
		{
			ROM_write(&memory, LE_COMBINE_BANK_SHORT(i, LoROM_ROM_BYTES[0] + j), rom[(i * byte_height) + j]);
		}
	}

	for(int i = 0; i < 21; i++)
	{
		printf("%c", DB_read(&memory, 0x00FFC0 + i));
	}

	printf("\n");

	struct Ricoh_5A22 cpu;
	reset_ricoh_5a22(&cpu, &memory);

	printf("%06x\n", LE_COMBINE_BANK_SHORT(cpu.program_bank, cpu.program_ctr));

	int i = 0;
	while(cpu.RDY && !cpu.LPM && i < 1000000000)
	{
		decode_execute(&cpu, &memory);
		if(memory.write_to_ROM)
		{
			uint32_t a = LE_COMBINE_BANK_SHORT(cpu.program_bank, cpu.program_ctr);
			for(int i = 16; i >= 0; i--)
			{
				printf("%02x ", DB_read(&memory, a - i));
			}
			printf("%06x\n", a);
		}

		i++;
	}

	return 0;
}
