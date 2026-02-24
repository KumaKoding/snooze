#include <stdlib.h>
#include <stdint.h>

#include "DMA.h"
#include "registers.h"
#include "memory.h"

void init_DMA(struct data_bus *data_bus)
{
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
//
// void MDMA_detect_parameters(struct data_bus *data_bus, int channel)
// {
// 	struct DMA *dma = data_bus->B_bus.dma;
//
// 	uint8_t pattern = mem_read(data_bus, swap_channels(DMA_PARAMETER, channel)) & 0b111;
// 	uint8_t adjust = mem_read(data_bus, swap_channels(DMA_PARAMETER, channel)) & 0b11000;
// 	uint8_t direction = mem_read(data_bus, swap_channels(DMA_PARAMETER, channel)) & 0b10000000;
//
// 	switch(pattern)
// 	{
// 		case 0: dma->transfer_pattern[channel] = P0; break;
//
// #define swap_channels(addr, n) (addr & 0xFFFFFF0F) | (n << 4) 
// 	dma->DMA_B_addr[channel] = B_addr;
// 	dma->HDMA_A_table_start_addr[channel] = table_addr;
// }
//
// void populate_MDMA(struct data_bus *data_bus)
// {
// 	struct DMA *dma = data_bus->B_bus.dma;
//
// 	for(int i = 0; i < N_CHANNELS; i++)
// 	{
// 		if((mem_read(data_bus, MDMAEN) >> i) & 0xFE)
// 		{
// 			dma->MDMA_enable[i] = 1;
// 			MDMA_detect_parameters(data_bus, i);
// 			MDMA_detect_addresses(data_bus, i);
// 		}
// 	}
// }
//
// void populate_HDMA(struct data_bus *data_bus)
// {
// 	struct DMA *dma = data_bus->B_bus.dma;
//
// 	for(int i = 0; i < N_CHANNELS; i++)
// 	{
// 		if((mem_read(data_bus, HDMAEN) >> i) & 0xFE)
// 		{
// 			dma->HDMA_enable[i] = 1;
// 			HDMA_detect_parameters(data_bus, i);
// 			HDMA_detect_addresses(data_bus, i);
// 		}
// 	}
// }
//
// void DMA_transfer_step(struct data_bus *data_bus)
// {
// 	// cannot transfer from wram -> wram
// 	for(int i = 0; i < N_CHANNELS; i++)
// 	{
//
// 	}
// }

