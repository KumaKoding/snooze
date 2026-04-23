#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "DMA.h"
#include "registers.h"
#include "memory.h"
#include "utility.h"

void init_DMA(struct data_bus *data_bus)
{
	data_bus->B_bus.dma->queued_cycles = 0;
	data_bus->B_bus.dma->elapsed_cycles = 0;
	data_bus->B_bus.dma->HDMA_allowed = 0;
	data_bus->B_bus.dma->dma_active = 0;

	data_bus->B_bus.dma->MDMA_channel_over = 1;
	data_bus->B_bus.dma->new_MDMA_transfer = 0;

	memset(data_bus->B_bus.dma->HDMA_channels_finished, 1, N_CHANNELS);
	memset(data_bus->B_bus.dma->HDMA_need_indirect, 1, N_CHANNELS);

	mem_write(data_bus, MDMAEN, 0x00);
	mem_write(data_bus, HDMAEN, 0x00);

	for(int i = 0; i < N_CHANNELS; i++)
	{
		mem_write(data_bus, swap_channels(DMAPx, i), 0xFF);

		mem_write(data_bus, swap_channels(BBADx, i), 0xFF);
		mem_write(data_bus, swap_channels(A1TxL, i), 0xFF);
		mem_write(data_bus, swap_channels(A1TxH, i), 0xFF);
		mem_write(data_bus, swap_channels(A1Bx, i), 0xFF);

		mem_write(data_bus, swap_channels(DASxL, i), 0xFF);
		mem_write(data_bus, swap_channels(DASxH, i), 0xFF);
		mem_write(data_bus, swap_channels(DASBx, i), 0xFF);

		mem_write(data_bus, swap_channels(A2AxL, i), 0xFF);
		mem_write(data_bus, swap_channels(A2AxH, i), 0xFF);

		mem_write(data_bus, swap_channels(NLTRx, i), 0xFF);

		mem_write(data_bus, swap_channels(UNUSED1, i), 0xFF);
		mem_write(data_bus, swap_channels(UNUSED2, i), 0xFF);
	}
}

uint32_t get_HDMA_table_index(struct DMA *dma, int channel)
{
	uint32_t addr = dma->DMA_source_addr[channel] & 0x00FF0000;
	addr |= dma->HDMA_A_table_index[channel];

	return addr;
}

void HDMA_transfer_byte(struct data_bus *data_bus, int channel, uint32_t *A_addr, uint32_t B_addr)
{	
	struct DMA *dma = data_bus->B_bus.dma;

	if(dma->direction[channel] == A_to_B)
	{
		mem_write(data_bus, B_addr, mem_read(data_bus, *A_addr));
	}
	else 
	{
		mem_write(data_bus, *A_addr, mem_read(data_bus, B_addr));
	}

	(*A_addr)++;

	dma->queued_cycles += 8;
}

void HDMA_use_pattern(struct data_bus *data_bus, int channel)
{
	struct DMA *dma = data_bus->B_bus.dma;

	uint32_t A_addr = 0;
	uint32_t B_addr = 0x00002100 | dma->DMA_B_addr[channel];

	if(dma->HDMA_indirect[channel])
	{
		A_addr = dma->DMA_size_or_indirect[channel];
	}
	else 
	{
		A_addr = get_HDMA_table_index(dma, channel);
	}

	switch(dma->transfer_pattern[channel])
	{
		case P0:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);

			break;
		case P1:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);

			break;
		case P2:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);

			break;
		case P3:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);

			break;
		case P4:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 2);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 3);

			break;
		case P5:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);

			break;
		case P6:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);

			break;
		case P7:
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);
			HDMA_transfer_byte(data_bus, channel, &A_addr, B_addr + 1);

			break;
	}

	if(dma->HDMA_indirect[channel])
	{
		dma->DMA_size_or_indirect[channel] = A_addr;
	}
	else 
	{
		dma->HDMA_A_table_index[channel] = A_addr;
	}
}

void HDMA_transfer_bytes(struct data_bus *data_bus, int channel)
{
	struct DMA *dma = data_bus->B_bus.dma;

	if(dma->HDMA_scanline_counter[channel] == 0) // always clears to 0 when it needs a new one
	{
		uint8_t limiter = mem_read(data_bus, get_HDMA_table_index(dma, channel));
		dma->HDMA_A_table_index[channel]++;

		if(limiter == 0x00)
		{
			dma->HDMA_enable[channel] = 0;

			return;
		}
		else 
		{
			dma->HDMA_scanline_counter[channel] = limiter & 0b01111111;
			dma->HDMA_repeat[channel] = (limiter & 0b10000000) >> 7;

			if(dma->HDMA_indirect[channel])
			{
				uint16_t indirect_addr = 0x0000;
				indirect_addr |= mem_read(data_bus, get_HDMA_table_index(dma, channel));
				dma->HDMA_A_table_index[channel]++;
				indirect_addr |= mem_read(data_bus, get_HDMA_table_index(dma, channel));
				dma->HDMA_A_table_index[channel]++;

				dma->DMA_size_or_indirect[channel] |= indirect_addr;

				dma->queued_cycles += 16;
			}

			HDMA_use_pattern(data_bus, channel);
		}
	}
	else 
	{
		if(dma->HDMA_repeat[channel])
		{
			HDMA_use_pattern(data_bus, channel);
		}
	}

	dma->queued_cycles += 8;
}

void HDMA_step(struct data_bus *data_bus, int channel)
{
	struct DMA *dma = data_bus->B_bus.dma;

	HDMA_transfer_bytes(data_bus, channel);

	dma->HDMA_channels_finished[channel] = 1;
}

void transfer_byte(struct data_bus *data_bus, int channel, int b_offset)
{
	struct DMA *dma = data_bus->B_bus.dma;

	uint32_t size = dma->DMA_size_or_indirect[channel] & 0x0000FFFF;

	if(size == 0)
	{
		size = 0x10000;
	}

	switch (dma->direction[channel]) 
	{
		case A_to_B:
			mem_write(data_bus, (0x2100 | dma->DMA_B_addr[channel]) + b_offset, mem_read(data_bus, dma->DMA_source_addr[channel]));

			break;
		case B_to_A:
			mem_write(data_bus, dma->DMA_source_addr[channel], mem_read(data_bus, (0x2100 | dma->DMA_B_addr[channel]) + b_offset));

			break;
	}

	switch (dma->MDMA_address_adjust[channel]) 
	{
		case Increment_A:
			dma->DMA_source_addr[channel]++;

			break;
		case Fixed:

			break;
		case Decrement_A:
			dma->DMA_source_addr[channel]--;

			break;
	}

	size--;

	dma->DMA_size_or_indirect[channel] &= 0xFFFF0000;
	dma->DMA_size_or_indirect[channel] |= size;

	dma->queued_cycles += 8;
}

void MDMA_step(struct data_bus *data_bus, int channel)
{
	struct DMA *dma = data_bus->B_bus.dma;

	if(dma->MDMA_channel_over)
	{
		printf("NEW DMA\n");
		dma->queued_cycles += 8;
	}

	dma->MDMA_channel_over = 0;


	switch(dma->transfer_pattern[channel])
	{
		case P0:
			transfer_byte(data_bus, channel, 0);

			break;
		case P1:
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 1);

			break;
		case P2:
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 0);

			break;
		case P3:
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 1);
			transfer_byte(data_bus, channel, 1);

			break;
		case P4:
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 1);
			transfer_byte(data_bus, channel, 2);
			transfer_byte(data_bus, channel, 3);

			break;
		case P5:
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 1);
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 1);

			break;
		case P6:
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 0);

			break;
		case P7:	
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 0);
			transfer_byte(data_bus, channel, 1);
			transfer_byte(data_bus, channel, 1);

			break;
	}

	if((dma->DMA_size_or_indirect[channel] & 0x0000FFFF) <= 0)
	{
		dma->MDMA_enable[channel] = 0;
		dma->MDMA_channel_over = 1;
	}
}

int check_new_HDMA(struct DMA *dma)
{
	int check = 1;

	for(int i = 0; i < N_CHANNELS; i++)
	{
		if(dma->HDMA_channels_finished[i] != 0)
		{
			check = 0;
		}
	}

	return check;
}

void DMA_transfers(struct data_bus *data_bus, int alignment)
{
	struct DMA *dma = data_bus->B_bus.dma;

	// sync multiple = 0 
	// transfer everything
	// add multiple corrector to next opcode
	// almost the same thing


	// cannot transfer from wram -> wram
	int priority_HDMA = -1;
	int priority_MDMA = -1;

	int i = 0;

	while(i < N_CHANNELS)
	{
		if(dma->HDMA_enable[i] && dma->HDMA_allowed && priority_HDMA == -1 && dma->HDMA_channels_finished[i] == 0)
		{
			priority_HDMA = i;

			if(!dma->dma_active)
			{
				dma->queued_cycles += 8 - alignment;
			}

			dma->dma_active = 1;
			dma->MDMA_enable[i] = 0;
			dma->MDMA_channel_over = 1;
		}
		else if(dma->MDMA_enable[i] && priority_MDMA == -1)
		{
			priority_MDMA = i;

			if(!dma->dma_active)
			{
				dma->queued_cycles += 8 - alignment;
			}

			dma->dma_active = 1;
		}

		i++;
	}

	if(priority_HDMA > -1)
	{
		if(check_new_HDMA(dma))
		{
			dma->queued_cycles += 18;
		}

		HDMA_step(data_bus, priority_HDMA);
	}
	else if(priority_MDMA > -1)
	{
		if(dma->new_MDMA_transfer)
		{
			dma->queued_cycles += 8;
			dma->new_MDMA_transfer = 0;
		}

		MDMA_step(data_bus, priority_MDMA);
	}


	if(dma->queued_cycles == 0)
	{
		dma->dma_active = 0;
		dma->elapsed_cycles = 0;
	}

	dma->elapsed_cycles += dma->queued_cycles;
}

