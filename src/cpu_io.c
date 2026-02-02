#include "memory.h"
#include "ricoh5A22.h"
#include <stdint.h>

uint8_t DB_read(struct data_bus *data_bus, uint32_t addr)
{	
	data_bus->A_Bus.cpu->queued_cyles++;

	return mem_read(data_bus, addr);
}

void DB_write(struct data_bus *data_bus, uint32_t index, uint8_t write_val)
{
	data_bus->A_Bus.cpu->queued_cyles++;

	mem_write(data_bus, index, write_val);
}

