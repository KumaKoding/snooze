#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "Ricoh5A22.h"
#include "memory.h"

int main()
{
	// uint8_t *memory = malloc(sizeof(uint8_t) * 0xFFFFFF);
	//
	// memory[RESET_VECTOR_6502[0]] = 0x00;
	// memory[RESET_VECTOR_6502[1]] = 0x00;
	//
	// memory[0x00] = OPCODE_ADC_IMM;
	// memory[0x01] = 0x03;
	//
	// struct Ricoh_5A22 cpu;
	// cpu.register_A = 0x40;
	//
	// init_ricoh_5a22(&cpu, memory);
	//
	// printf("%02x%04x\n", cpu.program_bank, cpu.program_ctr);
	//
	// cpu.cpu_status &= ~CPU_STATUS_M;
	//
	// decode_execute(&cpu, memory);
	//
	// printf("%08x %u\n", cpu.register_A, cpu.register_A);
	
	struct memory memory;
	init_memory(&memory, LoROM_MARKER);

	memory_indexer(&memory, 0x808000);
	memory_indexer(&memory, 0x80FFFF);
	memory_indexer(&memory, 0xFF8000);
	memory_indexer(&memory, 0xFFFFFF);
	printf("\n");

	memory_indexer(&memory, 0xC08000);
	memory_indexer(&memory, 0xC0FFFF);
	memory_indexer(&memory, 0xEFFFFF);
	memory_indexer(&memory, 0xEFFFFF);
	printf("\n");

	memory_indexer(&memory, 0xC00000);
	memory_indexer(&memory, 0xC07FFF);
	memory_indexer(&memory, 0xEF0000);
	memory_indexer(&memory, 0xEF7FFF);
	printf("\n");

	memory_indexer(&memory, 0x808000);
	memory_indexer(&memory, 0x80FFFF);
	memory_indexer(&memory, 0xFD8000);
	memory_indexer(&memory, 0xFDFFFF);
	printf("\n");

	memory_indexer(&memory, 0x008000);
	memory_indexer(&memory, 0x00FFFF);
	memory_indexer(&memory, 0x7D8000);
	memory_indexer(&memory, 0x7DFFFF);
	printf("\n");

	memory_indexer(&memory, 0x408000);
	memory_indexer(&memory, 0x40FFFF);
	memory_indexer(&memory, 0x6FFFFF);
	memory_indexer(&memory, 0x6FFFFF);
	printf("\n");


	memory_indexer(&memory, 0x408000);
	memory_indexer(&memory, 0x40FFFF);
	memory_indexer(&memory, 0x6F8000);
	memory_indexer(&memory, 0x6FFFFF);
	printf("\n");

	memory_indexer(&memory, 0xF00000);
	memory_indexer(&memory, 0xF07FFF);
	memory_indexer(&memory, 0xFD0000);
	memory_indexer(&memory, 0xFD7FFF);
	printf("\n");

	memory_indexer(&memory, 0xFE0000);
	memory_indexer(&memory, 0xFE7FFF);
	memory_indexer(&memory, 0xFF0000);
	memory_indexer(&memory, 0xFF7FFF);
	printf("\n");

	return 0;

}
