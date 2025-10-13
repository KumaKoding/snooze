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
	ROM_write(&memory, RESET_VECTOR_6502[0], 0x00);
	ROM_write(&memory, RESET_VECTOR_6502[1], 0x80);

	uint32_t program_start = 
		LE_COMBINE_BANK_2BYTE(
				0x00, 
				DB_read(&memory, RESET_VECTOR_6502[0]), 
				DB_read(&memory, RESET_VECTOR_6502[1])
		);

	struct Ricoh_5A22 cpu;
	init_ricoh_5a22(&cpu, &memory);

	cpu.cpu_status &= ~CPU_STATUS_M;

	cpu.register_A = 0;

	ROM_write(&memory, program_start, OPCODE_ADC_IMM);
	ROM_write(&memory, program_start + 1, 0x10);
	ROM_write(&memory, program_start + 2, 0x10);

	decode_execute(&cpu, &memory);

	printf("%d\n", cpu.register_A);


	return 0;

}
