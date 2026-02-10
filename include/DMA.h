#ifndef DMA_H
#define DMA_H

#include "memory.h"
#include <stdint.h>

// DMA 

#define N_CHANNELS 8

enum Pattern
{
	P0,
	P1,
	P2,
	P3,
	P4,
	P5,
	P6,
	P7
};

enum Address_adjust
{
	Increment_A,
	Fixed,
	Decrement_A
};

enum Direction
{
	A_to_B,
	B_to_A
};

struct DMA
{
	enum Pattern transfer_pattern[N_CHANNELS];
	enum Direction direction[N_CHANNELS];
	uint32_t DMA_B_addr[N_CHANNELS];

	enum Address_adjust MDMA_address_adjust[N_CHANNELS];
	uint32_t MDMA_A_addr[N_CHANNELS];
	uint16_t MDMA_bytes_to_transfer[N_CHANNELS];

	uint32_t HDMA_A_table_start_addr[N_CHANNELS];
	uint32_t HDMA_A_indirect_addr[N_CHANNELS];
	uint16_t HDMA_A_table_index[N_CHANNELS];
	uint8_t HDMA_scanline_counter[8];
	int HDMA_indirect[N_CHANNELS];
	int HDMA_repeat[N_CHANNELS];
};

// DMA ADDRESSES

// void init_DMA(struct data_bus *data_bus);
// void populate_MDMA(struct data_bus *data_bus);
// void populate_HDMA(struct data_bus *data_bus);
// void begin_DMA(struct Memory *memory);

#endif // DMA_H
