#ifndef DMA_H
#define DMA_H

#include "memory.h"
#include "registers.h"
#include <stdint.h>

// DMA 

#define N_CHANNELS 8
#define B_BUS_ADDR 0x00002100

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
	int MDMA_enable[N_CHANNELS];
	int HDMA_enable[N_CHANNELS];
	
	uint8_t param_byte[N_CHANNELS];
	enum Pattern transfer_pattern[N_CHANNELS];
	enum Direction direction[N_CHANNELS];
	uint8_t DMA_B_addr[N_CHANNELS];
	uint32_t DMA_source_addr[N_CHANNELS];
	uint32_t DMA_size_or_indirect[N_CHANNELS];

	enum Address_adjust MDMA_address_adjust[N_CHANNELS];

	uint16_t HDMA_A_table_index[N_CHANNELS];
	uint8_t HDMA_scanline_counter[N_CHANNELS];
	int HDMA_indirect[N_CHANNELS];
	int HDMA_repeat[N_CHANNELS];


	int HDMA_channels_finished[8];
	int HDMA_need_indirect[8];

	int MDMA_channel_over;
	int new_MDMA_transfer;

	int HDMA_allowed;
	int queued_cycles;
	int elapsed_cycles;
	int dma_active;

	int alignment_counter;
};


void init_DMA(struct data_bus *data_bus);
void DMA_transfers(struct data_bus *data_bus, int alignment);

#endif // DMA_H
