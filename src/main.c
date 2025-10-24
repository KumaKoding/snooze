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

	cpu.register_A = 3;
	cpu.register_X = 0x100;
	cpu.register_Y = 0x150;

	ROM_write(&memory, program_start, OPCODE_MVP_XYC);
	ROM_write(&memory, program_start + 1, 0x00);
	ROM_write(&memory, program_start + 2, 0x01);

	DB_write(&memory, 0x000100, 0x01);
	DB_write(&memory, 0x0000ff, 0x02);
	DB_write(&memory, 0x0000fe, 0x03);
	DB_write(&memory, 0x0000fd, 0x04);
	DB_write(&memory, 0x0000fc, 0x05);

	decode_execute(&cpu, &memory);
	decode_execute(&cpu, &memory);
	decode_execute(&cpu, &memory);
	decode_execute(&cpu, &memory);
	decode_execute(&cpu, &memory);
	// decode_execute(&cpu, &memory);

	printf("%02x ", DB_read(&memory, 0x010150));
	printf("%02x ", DB_read(&memory, 0x01014f));
	printf("%02x ", DB_read(&memory, 0x01014e));
	printf("%02x ", DB_read(&memory, 0x01014d));
	printf("%02x\n", DB_read(&memory, 0x01014c));

	printf("%04x\n", cpu.register_A);
	printf("%04x\n", cpu.register_X);
	printf("%04x\n", cpu.register_Y);
	printf("%02x\n", cpu.data_bank);

	return 0;

}
