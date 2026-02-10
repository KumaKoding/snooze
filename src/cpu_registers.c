#include <stdint.h>
#include "memory.h"
#include "ricoh5A22.h"
#include "registers.h"

void write_cpu_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value)
{
	if(addr == NMITIEN)
	{

	}
}

void read_cpu_register(struct data_bus *data_bus, uint32_t addr)
{
}

