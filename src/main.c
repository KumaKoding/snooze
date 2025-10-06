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
	init_memory(&memory, ExHiROM_MARKER);

	printf("%d\n", HiROM_ROM_SIZE);

	memory_indexer(&memory, 0x803000);

	return 0;

}
