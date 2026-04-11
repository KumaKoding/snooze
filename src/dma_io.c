#include "memory.h"
#include "DMA.h"
#include "ricoh5A22.h"
#include "PPU.h"

void allow_HDMA(struct data_bus *data_bus)
{
	data_bus->B_bus.dma->HDMA_allowed = 1;
}

void disallow_HDMA(struct data_bus *data_bus)
{
	data_bus->B_bus.dma->HDMA_allowed = 0;
}

void sync_DMA(struct data_bus *data_bus, int cycles)
{
	if(data_bus->B_bus.dma->dma_active == 1)
	{
		data_bus->A_Bus.cpu->queued_cyles += cycles - (data_bus->B_bus.dma->elapsed_cycles % cycles); // https://github.com/gilligan/snesdev/blob/master/docs/timing.txt
																									  // https://forums.nesdev.org/viewtopic.php?t=8652
		// "The DMA runs on a 1/8 clock rate, so when DMA starts, the CPU has to wait N (where N > 0) cycles 
		// until it's aligned to the DMA clock. Then you have the DMA setup, then transfers, then it has to 
		// sync back to the CPU. That takes the last cycle's length (6, 8 or 12) and sleeps N cycles 
		// (where N > 0) until the total DMA time was an even multiple of that cycle's length (eg for 6; 
		// the total DMA time must be 6, 12, 18, 24 ...)"

		data_bus->B_bus.dma->dma_active = 0;
		data_bus->B_bus.dma->elapsed_cycles = 0;
	}
}

