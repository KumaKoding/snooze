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

	cpu.cpu_emulation6502 &= ~CPU_STATUS_E;
	cpu.cpu_status &= ~CPU_STATUS_M;
	cpu.register_A = 0x0001;
	cpu.data_bank = 0x7E;


	ROM_write(&memory, program_start, OPCODE_ASL_ABS);
	ROM_write(&memory, program_start + 1, 0x00);
	ROM_write(&memory, program_start + 2, 0x00);

	DB_write(&memory, 0x7E0000, 0xFF);
	printf("%u\n", LE_COMBINE_2BYTE(DB_read(&memory, 0x7E0000), DB_read(&memory, 0x7E0001)));

	decode_execute(&cpu, &memory);
	printf("%u\n", LE_COMBINE_2BYTE(DB_read(&memory, 0x7E0000), DB_read(&memory, 0x7E0001)));



	return 0;

}
