#include "registers.h"
#include "memory.h"
#include "DMA.h"
#include <stdint.h>

static uint8_t check_bit8(uint8_t ps, uint8_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

void read_dma_register(struct data_bus *data_bus, uint32_t addr)
{

	write_register_raw(data_bus, addr, data_bus->open_value);
}

void write_dma_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value)
{
	struct DMA *dma = data_bus->B_bus.dma;

	if(addr == MDMAEN)
	{
		dma->MDMA_enable[7] = check_bit8(write_value, 0x80);
		dma->MDMA_enable[6] = check_bit8(write_value, 0x40);
		dma->MDMA_enable[5] = check_bit8(write_value, 0x20);
		dma->MDMA_enable[4] = check_bit8(write_value, 0x10);
		dma->MDMA_enable[3] = check_bit8(write_value, 0x08);
		dma->MDMA_enable[2] = check_bit8(write_value, 0x04);
		dma->MDMA_enable[1] = check_bit8(write_value, 0x02);
		dma->MDMA_enable[0] = check_bit8(write_value, 0x01);
	}

	if(addr == HDMAEN)
	{
		dma->HDMA_enable[7] = check_bit8(write_value, 0x80);
		dma->HDMA_enable[6] = check_bit8(write_value, 0x40);
		dma->HDMA_enable[5] = check_bit8(write_value, 0x20);
		dma->HDMA_enable[4] = check_bit8(write_value, 0x10);
		dma->HDMA_enable[3] = check_bit8(write_value, 0x08);
		dma->HDMA_enable[2] = check_bit8(write_value, 0x04);
		dma->HDMA_enable[1] = check_bit8(write_value, 0x02);
		dma->HDMA_enable[0] = check_bit8(write_value, 0x01);
	}

	if(in_DMA_addr(addr, DMAPx))
	{
		switch (write_value & 0x00000111) 
		{
			case 0:
				dma->transfer_pattern[get_channel(addr)] = P0; 

				break;
			case 1:
				dma->transfer_pattern[get_channel(addr)] = P1; 

				break;
			case 2:
				dma->transfer_pattern[get_channel(addr)] = P2; 

				break;
			case 3:
				dma->transfer_pattern[get_channel(addr)] = P3; 

				break;
			case 4:
				dma->transfer_pattern[get_channel(addr)] = P4; 

				break;
			case 5:
				dma->transfer_pattern[get_channel(addr)] = P5; 

				break;
			case 6:
				dma->transfer_pattern[get_channel(addr)] = P6; 

				break;
			case 7:
				dma->transfer_pattern[get_channel(addr)] = P7; 

				break;
		}

		switch((write_value & 0b00011000) >> 3)
		{
			case 0:
				dma->MDMA_address_adjust[get_channel(addr)] = Increment_A;

				break;
			case 1:
				dma->MDMA_address_adjust[get_channel(addr)] = Fixed;

				break;
			case 2:
				dma->MDMA_address_adjust[get_channel(addr)] = Decrement_A;

				break;
			case 3:
				dma->MDMA_address_adjust[get_channel(addr)] = Fixed;

				break;
		}

		dma->HDMA_indirect[get_channel(addr)] = check_bit8(write_value, 0x20);

		switch ((write_value & 0b11000000) >> 6) 
		{
			case 0: 
				dma->direction[get_channel(addr)] = A_to_B;

				break;
			case 1: 
				dma->direction[get_channel(addr)] = B_to_A;

				break;
		}
	}

	if(in_DMA_addr(addr, BBADx))
	{
		dma->DMA_B_addr[get_channel(addr)] = write_value;
	}

	if(in_DMA_addr(addr, A1TxL))
	{
		dma->MDMA_A_addr[get_channel(addr)] &= 0xFFFFFF00;
		dma->MDMA_A_addr[get_channel(addr)] |= write_value;

		dma->HDMA_A_table_start_addr[get_channel(addr)] &= 0xFFFFFF00;
		dma->HDMA_A_table_start_addr[get_channel(addr)] |= write_value;
	}

	if(in_DMA_addr(addr, A1TxH))
	{
		dma->MDMA_A_addr[get_channel(addr)] &= 0xFFFF00FF;
		dma->MDMA_A_addr[get_channel(addr)] |= (0x00000000 | write_value) << 8;

		dma->HDMA_A_table_start_addr[get_channel(addr)] &= 0xFFFF00FF;
		dma->HDMA_A_table_start_addr[get_channel(addr)] |= (0x00000000 | write_value) << 8;
	}

	if(in_DMA_addr(addr, A1Bx))
	{
		dma->MDMA_A_addr[get_channel(addr)] &= 0xFF00FFFF;
		dma->MDMA_A_addr[get_channel(addr)] |= (0x00000000 | write_value) << 16;

		dma->HDMA_A_table_start_addr[get_channel(addr)] &= 0xFF00FFFF;
		dma->HDMA_A_table_start_addr[get_channel(addr)] |= (0x00000000 | write_value) << 16;
	}

	if(in_DMA_addr(addr, DASxL))
	{
		// 0 = 65536 bytes
		dma->MDMA_bytes_to_transfer[get_channel(addr)] &= 0xFF00;
		dma->MDMA_bytes_to_transfer[get_channel(addr)] |= write_value;

		dma->HDMA_A_indirect_addr[get_channel(addr)] &= 0xFFFFFF00;
		dma->HDMA_A_indirect_addr[get_channel(addr)] |= write_value;
	}

	if(in_DMA_addr(addr, DASxH))
	{
		// 0 = 65536 bytes
		dma->MDMA_bytes_to_transfer[get_channel(addr)] &= 0x00FF;
		dma->MDMA_bytes_to_transfer[get_channel(addr)] |= (0x0000 | write_value) << 8;

		dma->HDMA_A_indirect_addr[get_channel(addr)] &= 0xFFFF00FF;
		dma->HDMA_A_indirect_addr[get_channel(addr)] |= (0x00000000 | write_value) << 8;
	}

	if(in_DMA_addr(addr, DASBx))
	{
		dma->HDMA_A_indirect_addr[get_channel(addr)] &= 0xFF00FFFF;
		dma->HDMA_A_indirect_addr[get_channel(addr)] |= (0x00000000 | write_value) << 16;
	}

	if(in_DMA_addr(addr, A2AxL))
	{
		dma->HDMA_A_table_index[get_channel(addr)] &= 0xFF00;
		dma->HDMA_A_table_index[get_channel(addr)] |= write_value;
	}

	if(in_DMA_addr(addr, A2AxH))
	{
		dma->HDMA_A_table_index[get_channel(addr)] &= 0x00FF;
		dma->HDMA_A_table_index[get_channel(addr)] |= (0x0000 | write_value) << 8;
	}

	if(in_DMA_addr(addr, NLTRx))
	{
		dma->HDMA_repeat[get_channel(addr)] = check_bit8(write_value, 0x80);
		dma->HDMA_scanline_counter[get_channel(addr)] = write_value * 0b011111111;
	}
}
