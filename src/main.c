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
	cpu.cpu_status &= ~CPU_STATUS_X;
	cpu.cpu_status |= CPU_STATUS_D;

	cpu.register_A = 0x3034;

	ROM_write(&memory, program_start, OPCODE_SBC_IMM);
	ROM_write(&memory, program_start + 1, 0x30);
	ROM_write(&memory, program_start + 2, 0x21);

	decode_execute(&cpu, &memory);

	printf("%u\n", cpu.register_A);
	printf("%04x\n", cpu.register_A);

	uint8_t c = cpu.cpu_status;

	printf("%c", ((c >> 7) & 0b00000001) ? 'N' : 'n');
	printf("%c", ((c >> 6) & 0b00000001) ? 'V' : 'v');
	printf("%c", ((c >> 5) & 0b00000001) ? 'M' : 'm');
	printf("%c", ((c >> 4) & 0b00000001) ? 'X' : 'x');
	printf("%c", ((c >> 3) & 0b00000001) ? 'D' : 'd');
	printf("%c", ((c >> 2) & 0b00000001) ? 'I' : 'i');
	printf("%c", ((c >> 1) & 0b00000001) ? 'Z' : 'z');
	printf("%c", ((c >> 0) & 0b00000001) ? 'C' : 'c');
	printf("\n");

	return 0;

}
