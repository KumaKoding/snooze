#include <stdlib.h>
#include <stdint.h>

#include "DMA.h"
#include "memory.h"

void init_DMA(struct Memory *memory)
{
	mem_write(memory, MDMA_ENABLE, 0x00);
	mem_write(memory, HDMA_ENABLE, 0x00);

	for(int i = 0; i < N_CHANNELS; i++)
	{
		mem_write(memory, swap_channels(DMA_PARAMETER, i), 0xFF);

		mem_write(memory, swap_channels(B_BUS_ADDR, i), 0xFF);

		mem_write(memory, swap_channels(A1_TRANSFER_ADDR[0], i), 0xFF);
		mem_write(memory, swap_channels(A1_TRANSFER_ADDR[1], i), 0xFF);
		mem_write(memory, swap_channels(A1_TRANSFER_ADDR[2], i), 0xFF);

		mem_write(memory, swap_channels(HDMA_CURRENT_EFF_ADDR[0], i), 0xFF);
		mem_write(memory, swap_channels(HDMA_CURRENT_EFF_ADDR[1], i), 0xFF);
		mem_write(memory, swap_channels(HDMA_CURRENT_EFF_ADDR[2], i), 0xFF);

		mem_write(memory, swap_channels(HDMA_TABBLE_ADDR[0], i), 0xFF);
		mem_write(memory, swap_channels(HDMA_TABBLE_ADDR[1], i), 0xFF);

		mem_write(memory, swap_channels(HDMA_REPEAT_SCANLINE_CTR, i), 0xFF);

		mem_write(memory, swap_channels(UNUSED_BYTES[0], i), 0xFF);
		mem_write(memory, swap_channels(UNUSED_BYTES[1], i), 0xFF);
	}
}

void MDMA_detect_parameters(struct DMA *dma, struct Memory *memory, int channel)
{
	uint8_t pattern = mem_read(memory, swap_channels(DMA_PARAMETER, channel)) & 0b111;
	uint8_t adjust = mem_read(memory, swap_channels(DMA_PARAMETER, channel)) & 0b11000;
	uint8_t direction = mem_read(memory, swap_channels(DMA_PARAMETER, channel)) & 0b10000000;

	switch(pattern)
	{
		case 0: dma->transfer_pattern[channel] = P0; break;
		case 1: dma->transfer_pattern[channel] = P1; break;
		case 2: dma->transfer_pattern[channel] = P2; break;
		case 3: dma->transfer_pattern[channel] = P3; break;
		case 4: dma->transfer_pattern[channel] = P4; break;
	}

	if(adjust & 0b10000)
	{
		dma->MDMA_address_adjust[channel] = Decrement_A;
	}
	else 
	{
		dma->MDMA_address_adjust[channel] = Increment_A;
	}

	if(adjust & 0b01000)
	{
		dma->MDMA_address_adjust[channel] = Fixed;
	}

	if(direction)
	{
		dma->direction[channel] = B_to_A;
	}
	else 
	{
		dma->direction[channel] = A_to_B;
	}
}

void MDMA_detect_addresses(struct DMA *dma, struct Memory *memory, int channel)
{
	uint32_t B_addr = 0x00002100 | mem_read(memory, swap_channels(B_BUS_ADDR, channel));

	uint32_t A_addr = 0x00000000;
	A_addr = A_addr | mem_read(memory, swap_channels(A1_TRANSFER_ADDR[2], channel));
	A_addr = (A_addr << 8) | mem_read(memory, swap_channels(A1_TRANSFER_ADDR[1], channel));
	A_addr = (A_addr << 8) | mem_read(memory, swap_channels(A1_TRANSFER_ADDR[0], channel));

	uint16_t byte_count = 0x0000;
	byte_count = byte_count | mem_read(memory, swap_channels(MDMA_BYTE_COUNT[1], channel));
	byte_count = (byte_count << 8) | mem_read(memory, swap_channels(MDMA_BYTE_COUNT[0], channel));

	dma->DMA_B_addr[channel] = B_addr;
	dma->MDMA_A_addr[channel] = A_addr;
	dma->MDMA_bytes_to_transfer[channel] = byte_count;
}

void HDMA_detect_parameters(struct DMA *dma, struct Memory *memory, int channel)
{
	uint8_t pattern = mem_read(memory, swap_channels(DMA_PARAMETER, channel)) & 0b111;
	uint8_t indirect = mem_read(memory, swap_channels(DMA_PARAMETER, channel)) & 0b1000000;
	uint8_t direction = mem_read(memory, swap_channels(DMA_PARAMETER, channel)) & 0b10000000;

	switch(pattern)
	{
		case 0: dma->transfer_pattern[channel] = P0; break;
		case 1: dma->transfer_pattern[channel] = P1; break;
		case 2: dma->transfer_pattern[channel] = P2; break;
		case 3: dma->transfer_pattern[channel] = P3; break;
		case 4: dma->transfer_pattern[channel] = P4; break;
	}

	if(indirect)
	{
		dma->HDMA_indirect[channel] = 1;
	}
	else 
	{
		dma->HDMA_indirect[channel] = 0;
	}

	if(direction)
	{
		dma->direction[channel] = B_to_A;
	}
	else 
	{
		dma->direction[channel] = A_to_B;
	}
}

void HDMA_detect_addresses(struct DMA *dma, struct Memory *memory, int channel)
{
	uint32_t B_addr = 0x00002100 | mem_read(memory, swap_channels(B_BUS_ADDR, channel));

	uint32_t table_addr = 0x00000000;
	table_addr = table_addr | mem_read(memory, swap_channels(A1_TRANSFER_ADDR[2], channel));
	table_addr = (table_addr << 8) | mem_read(memory, swap_channels(A1_TRANSFER_ADDR[1], channel));
	table_addr = (table_addr << 8) | mem_read(memory, swap_channels(A1_TRANSFER_ADDR[0], channel));

	dma->DMA_B_addr[channel] = B_addr;
	dma->HDMA_A_table_start_addr[channel] = table_addr;
}

void populate_MDMA(struct DMA *dma, struct Memory *memory)
{
	for(int i = 0; i < N_CHANNELS; i++)
	{
		if((mem_read(memory, MDMA_ENABLE) >> i) & 0xFE)
		{
			dma->MDMA_enable[i] = 1;
			MDMA_detect_parameters(dma, memory, i);
			MDMA_detect_addresses(dma, memory, i);
		}
	}
}

void populate_HDMA(struct DMA *dma, struct Memory *memory)
{
	for(int i = 0; i < N_CHANNELS; i++)
	{
		if((mem_read(memory, HDMA_ENABLE) >> i) & 0xFE)
		{
			dma->HDMA_enable[i] = 1;
			HDMA_detect_parameters(dma, memory, i);
			HDMA_detect_addresses(dma, memory, i);
		}
	}
}

void DMA_transfer_step(struct DMA *dma, struct Memory *memory)
{
	// cannot transfer from wram -> wram
	for(int i = 0; i < N_CHANNELS; i++)
	{

	}
}

