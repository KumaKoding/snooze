#include <stdio.h>
#include <stdint.h>
#include "memory.h"
#include "ricoh5A22.h"
#include "PPU.h"
#include "registers.h"

#define LE_HBYTE16(u16) (uint8_t)((u16 & 0xFF00) >> 8)
#define LE_LBYTE16(u16) (uint8_t)(u16 & 0x00FF)

#define SWP_LE_LBYTE16(u16, u8) ((u16 & 0xFF00) | u8)
#define SWP_LE_HBYTE16(u16, u8) ((u16 & 0x00FF) | ((0x0000 | u8) << 8))
#define LE_COMBINE_2BYTE(b1, b2) (uint16_t)(((0x0000 | b2) << 8) | b1)

static uint8_t check_bit8(uint8_t ps, uint8_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

#define CPU_VERSION 0x02;

void write_cpu_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(addr == NMITIEN)
	{
		cpu->internal_registers.NMIEN = check_bit8(write_value, 0x80);

		switch ((write_value & 0b00110000) >> 4) 
		{
			case 0b00:
				cpu->internal_registers.IRQEN = DISABLE;

				break;
			case 0b01:
				cpu->internal_registers.IRQEN = ENABLEHTIME;
				
				break;
			case 0b10:
				cpu->internal_registers.IRQEN = ENABLEVTIME;

				break;
			case 0b11:
				cpu->internal_registers.IRQEN = ENABLEHVTIME;
				
				break;
		}

		cpu->internal_registers.joypad_autoread = check_bit8(write_value, 0x01);
	}

	if(addr == WRIO)
	{
		cpu->internal_registers.IO_port_byte = write_value;

		if(!check_bit8(write_value, 0x80))
		{
			latch_HVCT(data_bus);
		}
	}

	if(addr == HTIMEL)
	{
		cpu->internal_registers.horizontal_IRQ_target = SWP_LE_LBYTE16(cpu->internal_registers.horizontal_IRQ_target, write_value);
	}

	if(addr == HTIMEH)
	{
		cpu->internal_registers.horizontal_IRQ_target = SWP_LE_HBYTE16(cpu->internal_registers.horizontal_IRQ_target, write_value);
	}

	if(addr == VTIMEL)
	{
		cpu->internal_registers.vertical_IRQ_target = SWP_LE_LBYTE16(cpu->internal_registers.vertical_IRQ_target, write_value);
	}

	if(addr == VTIMEH)
	{
		cpu->internal_registers.vertical_IRQ_target= SWP_LE_HBYTE16(cpu->internal_registers.vertical_IRQ_target, write_value);
	}

	if(addr == MDMAEN)
	{
		cpu->internal_registers.MDMA_enable[7] = check_bit8(write_value, 0x80);
		cpu->internal_registers.MDMA_enable[6] = check_bit8(write_value, 0x40);
		cpu->internal_registers.MDMA_enable[5] = check_bit8(write_value, 0x20);
		cpu->internal_registers.MDMA_enable[4] = check_bit8(write_value, 0x10);
		cpu->internal_registers.MDMA_enable[3] = check_bit8(write_value, 0x08);
		cpu->internal_registers.MDMA_enable[2] = check_bit8(write_value, 0x04);
		cpu->internal_registers.MDMA_enable[1] = check_bit8(write_value, 0x02);
		cpu->internal_registers.MDMA_enable[0] = check_bit8(write_value, 0x01);
	}

	if(addr == HDMAEN)
	{
		cpu->internal_registers.HDMA_enable[7] = check_bit8(write_value, 0x80);
		cpu->internal_registers.HDMA_enable[6] = check_bit8(write_value, 0x40);
		cpu->internal_registers.HDMA_enable[5] = check_bit8(write_value, 0x20);
		cpu->internal_registers.HDMA_enable[4] = check_bit8(write_value, 0x10);
		cpu->internal_registers.HDMA_enable[3] = check_bit8(write_value, 0x08);
		cpu->internal_registers.HDMA_enable[2] = check_bit8(write_value, 0x04);
		cpu->internal_registers.HDMA_enable[1] = check_bit8(write_value, 0x02);
		cpu->internal_registers.HDMA_enable[0] = check_bit8(write_value, 0x01);
	}

	if(addr == MEMSEL)
	{
		cpu->internal_registers.fast_ROM = check_bit8(write_value, 0x01);
	}
}

void read_cpu_register(struct data_bus *data_bus, uint32_t addr)
{
	struct Ricoh_5A22 *cpu = data_bus->A_Bus.cpu;

	if(addr == RDNMI)
	{
		uint8_t return_byte = (uint8_t)cpu->internal_registers.NMI_flag << 7;
		return_byte |= cpu->internal_registers.interal_read_bus & 0b01110000;
		return_byte |= cpu->internal_registers.cpu_version;

		write_register_raw(data_bus, addr, return_byte);

		cpu->internal_registers.NMI_flag = 0;
	}

	if(addr == TIMEUP)
	{
		uint8_t return_byte = (uint8_t)cpu->internal_registers.IRQ_flag << 7;
		return_byte |= cpu->internal_registers.interal_read_bus & 0x7F;

		write_register_raw(data_bus, addr, return_byte);

		cpu->internal_registers.IRQ_flag = 0;
	}

	if(addr == HVBJOY)
	{
		uint8_t return_byte = cpu->internal_registers.Vblank_flag << 7;
		return_byte |= cpu->internal_registers.Hblank_flag << 6;
		return_byte |= cpu->internal_registers.joypad_autoread_flag;

		write_register_raw(data_bus, addr, return_byte);
	}

	if(addr == RDIO)
	{
		write_register_raw(data_bus, addr, cpu->internal_registers.IO_port_byte);
	}
}

