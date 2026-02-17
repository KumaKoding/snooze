#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define MEMORY_AREA(x_0, y_0, x_1, y_1) (((x_1 + 1) - x_0) * ((y_1 + 1) - y_0))
#define WITHIN_REGION(i, bank_0, bytes_0, bank_1, bytes_1) \
	((((i & 0x00FF0000) >> 16) >= bank_0) && \
	(((i & 0x00FF0000) >> 16) <= bank_1) && \
	((i & 0x0000FFFF) >= bytes_0) && \
	((i & 0x0000FFFF) <= bytes_1))

// GENERAL ADDRESSES

#define WRAM_SIZE 128 * 1024
#define WRAM_BANKS (uint8_t[]){ 0x7E, 0x7F }
#define WRAM_BYTES (uint16_t[]){ 0x0000, 0xFFFF }
#define WRAM_LOWRAM_BYTES (uint16_t[]){ 0x0000, 0x1FFF }
#define WRAM_LOWRAM_MIRROR_BANKS (uint8_t[]) { 0x00, 0x3F }

#define REG_BANKS (uint8_t[]){ 0x00, 0x3F }
#define REG_BYTES (uint16_t[]){ 0x2000, 0x5FFF }
#define REG_SIZE \
	MEMORY_AREA(REG_BANKS[0], REG_BYTES[0], \
				REG_BANKS[1], REG_BYTES[1])

// LoROM ADDRESSES

#define LoROM_MARKER 0x20

#define LoROM_SRAM_BANKS (uint8_t[]){ 0xF0, 0xFD }
#define LoROM_SRAM_BYTES (uint16_t[]){ 0x0000, 0x7FFF }
#define LoROM_SRAM_MIRROR_BANKS (uint8_t[]){ 0xFE, 0xFF }
#define LoROM_SRAM_SIZE \
	MEMORY_AREA(LoROM_SRAM_BANKS[0], LoROM_SRAM_BYTES[0], \
				LoROM_SRAM_BANKS[1], LoROM_SRAM_BYTES[1])

#define LoROM_ROM_BANKS (uint8_t[]){ 0x80, 0xFF }
#define LoROM_ROM_BYTES (uint16_t[]){ 0x8000, 0xFFFF }
#define LoROM_ROM_MIRROR_BANKS (uint8_t[]){ 0xC0, 0xEF }
#define LoROM_ROM_MIRROR_BYTES (uint16_t[]){ 0x0000, 0x7FFF }
#define LoROM_ROM_SIZE \
	MEMORY_AREA(LoROM_ROM_BANKS[0], LoROM_ROM_BYTES[0], \
				LoROM_ROM_BANKS[1], LoROM_ROM_BYTES[1])

#define IN_WRAM(i) \
	WITHIN_REGION(i, WRAM_BANKS[0], WRAM_BYTES[0], \
					 WRAM_BANKS[1], WRAM_BYTES[1])

#define IN_WRAM_LOWRAM_MIRROR(i) \
	WITHIN_REGION(i, WRAM_LOWRAM_MIRROR_BANKS[0], WRAM_LOWRAM_BYTES[0], \
					 WRAM_LOWRAM_MIRROR_BANKS[1], WRAM_LOWRAM_BYTES[1])

#define IN_REG(i) \
	WITHIN_REGION(i, REG_BANKS[0], REG_BYTES[0], \
					 REG_BANKS[1], REG_BYTES[1])

#define IN_LoROM_ROM(i) \
	WITHIN_REGION(i, LoROM_ROM_BANKS[0], LoROM_ROM_BYTES[0], \
					 LoROM_ROM_BANKS[1], LoROM_ROM_BYTES[1])

#define IN_LoROM_ROM_MIRROR(i) \
	WITHIN_REGION(i, LoROM_ROM_MIRROR_BANKS[0], LoROM_ROM_MIRROR_BYTES[0], \
					 LoROM_ROM_MIRROR_BANKS[1], LoROM_ROM_MIRROR_BYTES[1])

#define IN_LoROM_SRAM(i) \
	WITHIN_REGION(i, LoROM_SRAM_BANKS[0], LoROM_SRAM_BYTES[0], \
					 LoROM_SRAM_BANKS[1], LoROM_SRAM_BYTES[1])

#define IN_LoROM_SRAM_MIRROR(i) \
	WITHIN_REGION(i, LoROM_SRAM_MIRROR_BANKS[0], LoROM_SRAM_BYTES[0], \
					 LoROM_SRAM_MIRROR_BANKS[1], LoROM_SRAM_BYTES[1])

// HiROM ADDRESSES

#define HiROM_MARKER 0x21

#define HiROM_SRAM_BANKS (uint8_t[]){ 0x30, 0x3F }
#define HiROM_SRAM_BYTES (uint16_t[]){ 0x6000, 0x7FFF }
#define HiROM_SRAM_SIZE MEMORY_AREA(HiROM_SRAM_BANKS[0], HiROM_SRAM_BYTES[0], HiROM_SRAM_BANKS[1], HiROM_SRAM_BYTES[1])

#define HiROM_MIRROR_1_BANKS (uint8_t[]){ 0x00, 0x3F }
#define HiROM_MIRROR_2_BANKS (uint8_t[]){ 0x80, 0xBF }
#define HiROM_MIRROR_BYTES (uint16_t[]){ 0x8000, 0xFFFF }
#define HiROM_MIRROR_1_SIZE MEMORY_AREA(HiROM_MIRROR_1_BANKS[0], HiROM_MIRROR_BYTES[0], HiROM_MIRROR_1_BANKS[1], HiROM_MIRROR_BYTES[1])
#define HiROM_MIRROR_2_SIZE MEMORY_AREA(HiROM_MIRROR_2_BANKS[0], HiROM_MIRROR_BYTES[0], HiROM_MIRROR_2_BANKS[1], HiROM_MIRROR_BYTES[1])

#define HiROM_ROM_BANKS (uint8_t[]){ 0xC0, 0xFF }
#define HiROM_ROM_BYTES (uint16_t[]){ 0x0000, 0xFFFF }
#define HiROM_ROM_SIZE MEMORY_AREA(HiROM_ROM_BANKS[0], HiROM_ROM_BYTES[0], HiROM_ROM_BANKS[1], HiROM_ROM_BYTES[1])

// ExHiROM ADDRESSES

#define ExHiROM_MARKER 0x25

#define ExHiROM_SRAM_BANKS (uint8_t[]){ 0x80, 0xBF }
#define ExHiROM_SRAM_BYTES (uint16_t[]){ 0x6000, 0x7FFF }
#define ExHiROM_SRAM_SIZE MEMORY_AREA(ExHiROM_SRAM_BANKS[0], ExHiROM_SRAM_BYTES[0], ExHiROM_SRAM_BANKS[1], ExHiROM_SRAM_BYTES[1])

#define ExHiROM_ROM_BANKS (uint8_t[]){ 0xC0, 0xFF }
#define ExHiROM_ROM_BYTES (uint16_t[]){ 0x0000, 0xFFFF }
#define ExHiROM_ROM_SIZE MEMORY_AREA(ExHiROM_ROM_BANKS[0], ExHiROM_ROM_BYTES[0], ExHiROM_ROM_BANKS[1], ExHiROM_ROM_BYTES[1])

#define ExHiROM_ROM_MIRROR_BANKS (uint8_t[]){ 0x80, 0xBF }
#define ExHiROM_ROM_MIRROR_BYTES (uint16_t[]){ 0x8000, 0xFFFF }
#define ExHiROM_ROM_MIRROR_SIZE MEMORY_AREA(ExHiROM_ROM_MIRROR_BANKS[0], ExHiROM_ROM_MIRROR_BYTES[0], ExHiROM_ROM_MIRROR_BANKS[1], ExHiROM_ROM_MIRROR_BYTES[1])

#define ExHiROM_ExROM_MIRROR_BANKS (uint8_t[]){ 0x00, 0x3D }
#define ExHiROM_ExROM_MIRROR_BYTES (uint16_t[]){ 0x8000, 0xFFFF }
#define ExHiROM_ExROM_MIRROR_SIZE MEMORY_AREA(ExHiROM_ExROM_MIRROR_BANKS[0], ExHiROM_ExROM_MIRROR_BYTES[0], ExHiROM_ExROM_MIRROR_BANKS[1], ExHiROM_ExROM_MIRROR_BYTES[1])

#define ExHiROM_ExROM_BANKS (uint8_t[]){ 0x3E, 0x3F, 0x40, 0x7D }
#define ExHiROM_ExROM_BYTES (uint16_t[]){ 0x8000, 0xFFFF, 0x0000, 0xFFFF }
#define ExHiROM_ExROM_SUBSIZE_1 MEMORY_AREA(ExHiROM_ExROM_BANKS[0], ExHiROM_ExROM_BYTES[0], ExHiROM_ExROM_BANKS[1], ExHiROM_ExROM_BYTES[1])
#define ExHiROM_ExROM_SUBSIZE_2 MEMORY_AREA(ExHiROM_ExROM_BANKS[2], ExHiROM_ExROM_BYTES[2], ExHiROM_ExROM_BANKS[3], ExHiROM_ExROM_BYTES[3])
#define ExHiROM_ExROM_SIZE ExHiROM_ExROM_SUBSIZE_1 + ExHiROM_ExROM_SUBSIZE_2

struct LoROM
{
	uint8_t *SRAM;
	uint8_t *ROM;
};

struct HiROM
{
	uint8_t *SRAM;
	uint8_t *ROM;
};

struct ExHiROM
{
	uint8_t *SRAM;
	uint8_t *ROM;
	uint8_t *ExROM;
};

union ROM_t
{
	struct LoROM LoROM;
	struct HiROM HiROM;
	struct ExHiROM ExHiROM;
};

struct Memory
{
	uint8_t *WRAM;
	uint32_t WRAM_addr; // https://snes.nesdev.org/wiki/WRAM_pinout
						// PS1, /PS5..1: Peripheral select, connected to PA7..2 to map S-WRAM to peripheral bus addresses $80-83.
						// PA1..0: These select the S-WRAM register being accessed on the peripheral bus (WRAM data or address).

	uint8_t *REG;

	uint8_t ROM_type_marker;
	union ROM_t ROM;
};

struct Ricoh_5A22;
struct PPU;
struct DMA;

struct data_bus
{
	struct 
	{
		struct S_PPU *ppu;
		struct DMA *dma;
	} B_bus;

	struct 
	{
		struct Ricoh_5A22 *cpu;
		struct Memory *memory;
	} A_Bus;

	uint8_t open_value;
};

void init_memory(struct Memory *memory, uint8_t ROM_type_marker);

uint8_t DB_read(struct data_bus *data_bus, uint32_t addr);
uint8_t mem_read(struct data_bus *data_bus, uint32_t addr);

void ROM_write(struct data_bus *data_bus, uint32_t addr, uint8_t val);
void write_register_raw(struct data_bus *data_bus, uint32_t addr, uint8_t val);
uint8_t read_register_raw(struct data_bus *data_bus, uint32_t addr);
void DB_write(struct data_bus *data_bus, uint32_t index, uint8_t write_val);
void mem_write(struct data_bus *data_bus, uint32_t index, uint8_t write_val);

void read_ppu_register(struct data_bus *data_bus, uint32_t addr);
void write_ppu_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value);
uint16_t read_VRAM(struct data_bus *data_bus, uint16_t addr);
void write_VRAM_word(struct data_bus *data_bus, uint16_t addr, uint16_t word);
void write_VRAM_low(struct data_bus *data_bus, uint16_t addr, uint8_t byte);
void write_VRAM_high(struct data_bus *data_bus, uint16_t addr, uint8_t byte);
uint8_t read_OAM(struct data_bus *data_bus, uint16_t addr);
void write_OAM(struct data_bus *data_bus, uint16_t addr, uint8_t byte);
void write_CGRAM_word(struct data_bus *data_bus, uint16_t addr, uint16_t word);
uint16_t read_CGRAM(struct data_bus *data_bus, uint16_t addr);

void read_wram_register(struct data_bus *data_bus, uint32_t addr);
void write_wram_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value);

void read_cpu_register(struct data_bus *data_bus, uint32_t addr);
void write_cpu_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value);

void signal_vblank(struct data_bus *data_bus);
void clear_vblank(struct data_bus *data_bus);
void signal_hblank(struct data_bus *data_bus);
void clear_hblank(struct data_bus *data_bus);
void check_IRQ(struct data_bus *data_bus);

#endif
