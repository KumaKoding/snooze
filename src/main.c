#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "Ricoh5A22.h"
#include "memory.h"

int main(int argc, char *argv[])
{
	struct Memory memory;
	init_memory(&memory, LoROM_MARKER);

	struct stat s;
	stat(argv[1], &s);
	FILE *cart = fopen(argv[1], "rb");

	uint8_t *rom = malloc(s.st_size * sizeof(uint8_t));
	fread(rom, s.st_size, sizeof(uint8_t), cart);

	int byte_height = LoROM_ROM_BYTES[1] - LoROM_ROM_BYTES[0];

	int banks = (int)ceilf((float)s.st_size / (float)byte_height);


	for(uint8_t i = 0; i < banks; i++)
	{
		for(uint16_t j = 0; j < byte_height && ((i * byte_height) + j) < s.st_size; j++)
		{
			ROM_write(&memory, LE_COMBINE_BANK_SHORT(i, LoROM_ROM_BYTES[0] + j), rom[(i * byte_height) + j]);
		}
	}
	//
	// for(int i = 0; i < 21; i++)
	// {
	// 	printf("%c", DB_read(&memory, 0x00FFC0 + i));
	// }
	//
	// printf("\n");

	struct Ricoh_5A22 cpu;

	cpu.register_X = 0;
	cpu.register_Y = 0;
	cpu.register_A = 0;
	cpu.stack_ptr = 0;
	cpu.direct_page = 0;
	cpu.data_bank = 0;

	reset_ricoh_5a22(&cpu, &memory);

	int i = 0;


	while(i < 50000)
	{
		if(cpu.LPM)
		{
			reset_ricoh_5a22(&cpu, &memory);
		}

		while(!cpu.RDY)
		{
			if(DB_read(&memory, 0x4210) == 0x42)
			{
				cpu.RDY = 1;
			}
		}

		print_cpu(&cpu);
		decode_execute(&cpu, &memory);

		i++;
	}

	return 0;
}
