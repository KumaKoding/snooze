#include "memory.h"
#include "PPU.h"
#include "registers.h"

#define LE_HBYTE16(u16) (uint8_t)((u16 & 0xFF00) >> 8)
#define LE_LBYTE16(u16) (uint8_t)(u16 & 0x00FF)

#define SWP_LE_LBYTE16(u16, u8) ((u16 & 0xFF00) | u8)
#define SWP_LE_HBYTE16(u16, u8) ((u16 & 0x00FF) | ((0x0000 | u8) << 8))
#define LE_COMBINE_2BYTE(b1, b2) (uint16_t)(((0x0000 | b2) << 8) | b1)

uint8_t check_bit8(uint8_t ps, uint8_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

uint8_t check_bit16(uint16_t ps, uint16_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

uint16_t read_VRAM(struct data_bus *data_bus, uint16_t addr)
{
	return LE_COMBINE_2BYTE(data_bus->B_bus.ppu->memory->VRAM[addr * 2], data_bus->B_bus.ppu->memory->VRAM[addr * 2 + 1]);
}

void write_VRAM_word(struct data_bus *data_bus, uint16_t addr, uint16_t word)
{
	data_bus->B_bus.ppu->memory->VRAM[addr * 2] = LE_LBYTE16(word);
	data_bus->B_bus.ppu->memory->VRAM[addr * 2 + 1] = LE_HBYTE16(word);
}

void write_VRAM_low(struct data_bus *data_bus, uint16_t addr, uint8_t byte)
{
	data_bus->B_bus.ppu->memory->VRAM[addr * 2] = byte;
}

void write_VRAM_high(struct data_bus *data_bus, uint16_t addr, uint8_t byte)
{
	data_bus->B_bus.ppu->memory->VRAM[addr * 2 + 1] = byte;
}

uint8_t read_OAM(struct data_bus *data_bus, uint16_t addr)
{
	if(check_bit16(addr, 0x0200))
	{
		return data_bus->B_bus.ppu->memory->OAM_high_table[addr & 0x01FF];
	}
	else 
	{
		return data_bus->B_bus.ppu->memory->OAM_low_table[addr & 0x01FF];
	}
}

void write_OAM(struct data_bus *data_bus, uint16_t addr, uint8_t byte)
{
	if(check_bit16(addr, 0x0200))
	{
		data_bus->B_bus.ppu->memory->OAM_high_table[addr & 0x01FF] = byte;
	}
	else 
	{
		data_bus->B_bus.ppu->memory->OAM_low_table[addr & 0x01FF] = byte;
	}
}

void write_CGRAM_word(struct data_bus *data_bus, uint16_t addr, uint16_t word)
{
	data_bus->B_bus.ppu->memory->CGRAM[addr * 2] = LE_LBYTE16(word);
	data_bus->B_bus.ppu->memory->CGRAM[addr * 2 + 1] = LE_HBYTE16(word);
}

uint16_t read_CGRAM(struct data_bus *data_bus, uint16_t addr)
{
	return LE_COMBINE_2BYTE(data_bus->B_bus.ppu->memory->CGRAM[addr * 2], data_bus->B_bus.ppu->memory->CGRAM[addr * 2 + 1]);
}

void write_ppu_register(struct data_bus *data_bus, uint32_t addr, uint8_t write_value)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(addr == INIDISP)
	{
		if(check_bit8(write_value, 0x80))
		{
			ppu->F_blank = 1;
		}
		else 
		{
			ppu->F_blank = 0;
		}

		ppu->brightness = write_value & 0b00001111;
	}

	if(addr == OBJSEL)
	{
		switch((write_value & 0b11100000) >> 5)
		{
			case 0:
				ppu->obj_sizes[0] = S_8x8;
				ppu->obj_sizes[1] = S_16x16;

				break;	
			case 1:
				ppu->obj_sizes[0] = S_8x8;
				ppu->obj_sizes[1] = S_32x32;

				break;
			case 2:
				ppu->obj_sizes[0] = S_8x8;
				ppu->obj_sizes[1] = S_64x64;

				break;
			case 3:
				ppu->obj_sizes[0] = S_16x16;
				ppu->obj_sizes[1] = S_32x32;

				break;
			case 4:
				ppu->obj_sizes[0] = S_16x16;
				ppu->obj_sizes[1] = S_64x64;

				break;
			case 5:
				ppu->obj_sizes[0] = S_32x32;
				ppu->obj_sizes[1] = S_64x64;

				break;
			case 6:
				ppu->obj_sizes[0] = S_16x32;
				ppu->obj_sizes[1] = S_32x64;

				break;
			case 7:
				ppu->obj_sizes[0] = S_16x32;
				ppu->obj_sizes[1] = S_32x32;

				break;
			default:
				break;
		}

		ppu->name_select = (write_value & 0b00011000) >> 3;
		ppu->name_base_addr = write_value & 0b00000111;
	}

	if(addr == BGMODE)
	{
		if(check_bit8(write_value, 0b10000000))
		{
			ppu->BGn_character_size[3] = CH_SIZE_16x16;
		}
		else
		{
			ppu->BGn_character_size[3] = CH_SIZE_8x8;
		}

		if(check_bit8(write_value, 0b01000000))
		{
			ppu->BGn_character_size[2] = CH_SIZE_16x16;
		}
		else 
		{
			ppu->BGn_character_size[2] = CH_SIZE_8x8;
		}

		if(check_bit8(write_value, 0b00100000))
		{
			ppu->BGn_character_size[1] = CH_SIZE_16x16;
		}
		else 
		{
			ppu->BGn_character_size[1] = CH_SIZE_8x8;
		}

		if(check_bit8(write_value, 0b00010000))
		{
			ppu->BGn_character_size[0] = CH_SIZE_16x16;
		}
		else 
		{
			ppu->BGn_character_size[0] = CH_SIZE_8x8;
		}

		ppu->M1_BG3_priority = check_bit8(write_value, 0b00001000);
		ppu->BG_mode = write_value & 0b00000111;
	}

	if(addr == BG1SC)
	{
		ppu->BGn_tilemap_info.tilemap_vram_addr[0] = write_value >> 2;
		ppu->BGn_tilemap_info.vertical_tilemaps[0] = check_bit8(write_value, 0b00000010) + 1;
		ppu->BGn_tilemap_info.horizontal_tilemaps[0] = check_bit8(write_value, 0b00000001) + 1;
	}

	if(addr == BG1SC)
	{
		ppu->BGn_tilemap_info.tilemap_vram_addr[1] = write_value >> 2;
		ppu->BGn_tilemap_info.vertical_tilemaps[1] = check_bit8(write_value, 0b00000010) + 1;
		ppu->BGn_tilemap_info.horizontal_tilemaps[1] = check_bit8(write_value, 0b00000001) + 1;
	}

	if(addr == BG1SC)
	{
		ppu->BGn_tilemap_info.tilemap_vram_addr[2] = write_value >> 2;
		ppu->BGn_tilemap_info.vertical_tilemaps[2] = check_bit8(write_value, 0b00000010) + 1;
		ppu->BGn_tilemap_info.horizontal_tilemaps[2] = check_bit8(write_value, 0b00000001) + 1;
	}

	if(addr == BG4SC)
	{
		ppu->BGn_tilemap_info.tilemap_vram_addr[3] = write_value >> 2;
		ppu->BGn_tilemap_info.vertical_tilemaps[3] = check_bit8(write_value, 0b00000010) + 1;
		ppu->BGn_tilemap_info.horizontal_tilemaps[3] = check_bit8(write_value, 0b00000001) + 1;
	}

	if(addr == BG12NBA)
	{
		ppu->BGn_chr_tiles_offset[1] = (write_value & 0b11110000) >> 4;
		ppu->BGn_chr_tiles_offset[0] = write_value & 0b00001111;
	}

	if(addr == BG34NBA)
	{
		ppu->BGn_chr_tiles_offset[3] = (write_value & 0b11110000) >> 4;
		ppu->BGn_chr_tiles_offset[2] = write_value & 0b00001111;
	}

	if(addr == BG1HOFS)
	{
		ppu->BG_scroll_offset.BGn_horizontal_offset[0] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_horizontal_offset[0] |= ppu->BG_scroll_offset.BG_offset_latch & 0b11111000;
		ppu->BG_scroll_offset.BGn_horizontal_offset[0] |= ppu->BG_scroll_offset.PPU2_horizontal_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
		ppu->BG_scroll_offset.PPU2_horizontal_latch = write_value & 0b00000111;
	}

	if(addr == BG1VOFS)
	{
		ppu->BG_scroll_offset.BGn_vertical_offset[0] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_vertical_offset[0] |= ppu->BG_scroll_offset.BG_offset_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
	}

	if(addr == BG2HOFS)
	{
		ppu->BG_scroll_offset.BGn_horizontal_offset[1] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_horizontal_offset[1] |= ppu->BG_scroll_offset.BG_offset_latch & 0b11111000;
		ppu->BG_scroll_offset.BGn_horizontal_offset[1] |= ppu->BG_scroll_offset.PPU2_horizontal_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
		ppu->BG_scroll_offset.PPU2_horizontal_latch = write_value & 0b00000111;
	}

	if(addr == BG2VOFS)
	{
		ppu->BG_scroll_offset.BGn_vertical_offset[1] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_vertical_offset[1] |= ppu->BG_scroll_offset.BG_offset_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
	}

	if(addr == BG3HOFS)
	{
		ppu->BG_scroll_offset.BGn_horizontal_offset[2] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_horizontal_offset[2] |= ppu->BG_scroll_offset.BG_offset_latch & 0b11111000;
		ppu->BG_scroll_offset.BGn_horizontal_offset[2] |= ppu->BG_scroll_offset.PPU2_horizontal_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
		ppu->BG_scroll_offset.PPU2_horizontal_latch = write_value & 0b00000111;
	}

	if(addr == BG3VOFS)
	{
		ppu->BG_scroll_offset.BGn_vertical_offset[2] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_vertical_offset[2] |= ppu->BG_scroll_offset.BG_offset_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
	}

	if(addr == BG4HOFS)
	{
		ppu->BG_scroll_offset.BGn_horizontal_offset[3] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_horizontal_offset[3] |= ppu->BG_scroll_offset.BG_offset_latch & 0b11111000;
		ppu->BG_scroll_offset.BGn_horizontal_offset[3] |= ppu->BG_scroll_offset.PPU2_horizontal_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
		ppu->BG_scroll_offset.PPU2_horizontal_latch = write_value & 0b00000111;
	}

	if(addr == BG4VOFS)
	{
		ppu->BG_scroll_offset.BGn_vertical_offset[3] = (0x0000 | write_value) << 8;
		ppu->BG_scroll_offset.BGn_vertical_offset[3] |= ppu->BG_scroll_offset.BG_offset_latch;

		ppu->BG_scroll_offset.BG_offset_latch = write_value;
	}

	if(addr == VMAIN)
	{
		ppu->VRAM_increment_mode = check_bit8(write_value, 0b10000000);

		switch ((write_value & 0b00001100) >> 2)
		{
			case 0:
				ppu->VRAM_remap = None;
				
				break;
			case 1:
				ppu->VRAM_remap = M_2bpp;

				break;
			case 2: 
				ppu->VRAM_remap = M_4bpp;
				
				break;
			case 3:
				ppu->VRAM_remap = M_8bpp;

				break;
			default:
				break;
		}

		switch (write_value & 0b00000011)
		{
			case 0:
				ppu->address_increment = 1;
				
				break;
			case 1:
				ppu->address_increment = 32;

				break;
			case 2: 
				ppu->address_increment = 128;
				
				break;
			case 3:
				ppu->address_increment = 128;

				break;
			default:
				break;
		}
	}

	if(addr == TM)
	{
		ppu->OBJ_main_enable = check_bit8(write_value, 0b00010000);
		ppu->BGn_main_enable[3] = check_bit8(write_value, 0b00001000);
		ppu->BGn_main_enable[2] = check_bit8(write_value, 0b00000100);
		ppu->BGn_main_enable[1] = check_bit8(write_value, 0b00000010);
		ppu->BGn_main_enable[0] = check_bit8(write_value, 0b00000001);
	}

	if(addr == COLDATA)
	{
		if(check_bit8(write_value, 0b10000000))
		{
			ppu->fixed_blue = write_value & 0b00011111;
		}

		if(check_bit8(write_value, 0b01000000))
		{
			ppu->fixed_green = write_value & 0b00011111;
		}

		if(check_bit8(write_value, 0b00100000))
		{
			ppu->fixed_red = write_value & 0b00011111;
		}
	}

	if(addr == SETINI)
	{
		ppu->external_image_sync = check_bit8(write_value, 0b10000000);
		ppu->M7_EXTBG = check_bit8(write_value, 0b01000000);
		ppu->high_res = check_bit8(write_value, 0b00001000);
		ppu->overscan = check_bit8(write_value, 0b00000100);
		ppu->OBJ_interlacing = check_bit8(write_value, 0b00000010);
		ppu->screen_interlacing = check_bit8(write_value, 0b00000001);
	}

	if(addr == OAMADDL)
	{
		ppu->OAM_addr = LE_COMBINE_2BYTE(mem_read(data_bus, OAMADDL), mem_read(data_bus, OAMADDH)) * 2;
	}

	if(addr == OAMADDH)
	{
		ppu->OAM_addr = LE_COMBINE_2BYTE(mem_read(data_bus, OAMADDL), mem_read(data_bus, OAMADDH)) * 2;
	}

	if(addr == OAMDATA)
	{
		if(!check_bit16(ppu->OAM_addr, 0x0001))
		{
			ppu->OAM_latch = write_value;
		}
		else if(ppu->OAM_addr < OAM_LTABLE_BYTES)
		{
			write_OAM(data_bus, ppu->OAM_addr - 1, ppu->OAM_latch);
			write_OAM(data_bus, ppu->OAM_addr, write_value);
		}
		
		if(ppu->OAM_addr >= OAM_LTABLE_BYTES)
		{
			write_OAM(data_bus, ppu->OAM_addr, write_value);
		}

		ppu->OAM_addr++;
	}

	if(addr == VMADDL)
	{
		// cannot read during blanks
		ppu->VRAM_addr = LE_COMBINE_2BYTE(mem_read(data_bus, VMADDL), mem_read(data_bus, VMADDH)) * 2;
		ppu->VRAM_latch = read_VRAM(data_bus, ppu->VRAM_addr);
	}

	if(addr == VMADDH)
	{
		// cannot read during blanks
		ppu->VRAM_addr = LE_COMBINE_2BYTE(mem_read(data_bus, VMADDL), mem_read(data_bus, VMADDH));
		ppu->VRAM_latch = read_VRAM(data_bus, ppu->VRAM_addr);
	}

	if(addr == VMDATAL)
	{
		write_VRAM_low(data_bus, ppu->VRAM_addr, write_value);

		if(ppu->VRAM_increment_mode == 0)
		{
			ppu->VRAM_addr++;
		}
	}

	if(addr == VMDATAH)
	{
		write_VRAM_high(data_bus, ppu->VRAM_addr, write_value);

		if(ppu->VRAM_increment_mode == 1)
		{
			ppu->VRAM_addr++;
		}
	}

	if(addr == CGADD)
	{
		ppu->CGRAM_addr = write_value;
		ppu->CGRAM_check = 0;
	}

	if(addr == CGDATA)
	{
		if(ppu->CGRAM_check)
		{
			ppu->CGRAM_latch = write_value;
			ppu->CGRAM_check = 1;
		}
		else 
		{
			write_CGRAM_word(data_bus, ppu->CGRAM_addr, LE_COMBINE_2BYTE(ppu->CGRAM_latch, write_value));
			ppu->CGRAM_check = 0;
		}
	}
}

void latch_HVCT(struct PPU *ppu)
{
	// put in wrio and joypad
	if(ppu->HV_latch == 0)
	{
		ppu->HV_latch = 1;

		ppu->hscan_counter = ppu->x / 2;
		ppu->vscan_counter = ppu->y / 2;
	}
}

void clear_HVCT(struct PPU *ppu)
{
	if(ppu->HV_latch == 1)
	{
		ppu->HV_latch = 0;
	}
}

void read_ppu_register(struct data_bus *data_bus, uint32_t addr)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(addr == MPYL) { ppu->PPU1_bus = 0x00; }
	if(addr == MPYM) { ppu->PPU1_bus = 0x00; }
	if(addr == MPYH) { ppu->PPU1_bus = 0x00; }

	if(addr == OAMDATAREAD)
	{
		uint8_t read = read_OAM(data_bus, ppu->OAM_addr);
		mem_write(data_bus, OAMDATAREAD, read);
		ppu->PPU1_bus = read;

		ppu->OAM_addr++;
	}

	if(addr == OAMDATAREAD) { mem_write(data_bus, OAMDATAREAD, ppu->PPU1_bus); }
	if(addr == BGMODE) { mem_write(data_bus, BGMODE, ppu->PPU1_bus); }
	if(addr == MOSAIC) { mem_write(data_bus, MOSAIC, ppu->PPU1_bus); }
	if(addr == BG2SC) { mem_write(data_bus, BG2SC, ppu->PPU1_bus); }
	if(addr == BG3SC) { mem_write(data_bus, BG3SC, ppu->PPU1_bus); }
	if(addr == BG4SC) { mem_write(data_bus, BG4SC, ppu->PPU1_bus); }

	if(addr == BG4VOFS) { mem_write(data_bus, BG4VOFS, ppu->PPU1_bus); }
	if(addr == VMAIN) { mem_write(data_bus, VMAIN, ppu->PPU1_bus); }
	if(addr == VMADDL) { mem_write(data_bus, VMADDL, ppu->PPU1_bus); }
	if(addr == VMDATAL) { mem_write(data_bus, VMDATAL, ppu->PPU1_bus); }
	if(addr == VMDATAH) { mem_write(data_bus, VMDATAH, ppu->PPU1_bus); }
	if(addr == M7SEL) { mem_write(data_bus, M7SEL, ppu->PPU1_bus); }

	if(addr == W34SEL) { mem_write(data_bus, W34SEL, ppu->PPU1_bus); }
	if(addr == WOBJSEL) { mem_write(data_bus, WOBJSEL, ppu->PPU1_bus); }
	if(addr == WH0) { mem_write(data_bus, WH0, ppu->PPU1_bus); }
	if(addr == WH2) { mem_write(data_bus, WH2, ppu->PPU1_bus); }
	if(addr == WH3) { mem_write(data_bus, WH3, ppu->PPU1_bus); }
	if(addr == WBGLOG) { mem_write(data_bus, WBGLOG, ppu->PPU1_bus); }

	if(addr == VMDATALREAD)
	{
		uint8_t read = LE_LBYTE16(ppu->VRAM_latch);
		mem_write(data_bus, VMDATALREAD, read);
		ppu->PPU1_bus = read;

		if(ppu->VRAM_increment_mode == 0)
		{
			ppu->VRAM_latch = read_VRAM(data_bus, ppu->VRAM_addr);
			ppu->VRAM_addr++;
		}
	}

	if(addr == VMDATAHREAD)
	{
		uint8_t read = LE_HBYTE16(ppu->VRAM_latch);
		mem_write(data_bus, VMDATALREAD, read);
		ppu->PPU1_bus = read;

		if(ppu->VRAM_increment_mode == 1)
		{
			ppu->VRAM_latch = read_VRAM(data_bus, ppu->VRAM_addr);
			ppu->VRAM_addr++;
		}
	}

	if(addr == CGDATAREAD)
	{

		if(ppu->CGRAM_check)
		{
			uint8_t read =  LE_HBYTE16(read_CGRAM(data_bus, ppu->CGRAM_addr));
			read = (read & (~0x80)) | (ppu->PPU2_bus & 0x80);
			mem_write(data_bus, CGDATAREAD, read);
			ppu->PPU2_bus = read;

			ppu->CGRAM_check = 0;
		}
		else
		{
			uint8_t read =  LE_LBYTE16(read_CGRAM(data_bus, ppu->CGRAM_addr));
			mem_write(data_bus, CGDATAREAD, read);
			ppu->PPU2_bus = read;

			ppu->CGRAM_check = 1;
		}
	}

	if(addr == SLHV)
	{
		if(mem_read(data_bus, WRIO) & 0x80)
		{
			latch_HVCT(ppu);
		}
	}

	if(addr == OPHCT)	
	{
		if(ppu->OPHCT_byte == 0)
		{
			uint8_t read = LE_LBYTE16(ppu->hscan_counter);
			mem_write(data_bus, addr, read);
			ppu->PPU2_bus = read;
		}
		else 
		{
			uint8_t read = LE_HBYTE16(ppu->hscan_counter);
			read = (read & 0x01) | (ppu->PPU2_bus & (~0x01));
			mem_write(data_bus, addr, read);
			ppu->PPU2_bus = read;
		}

		ppu->OPHCT_byte = ~ppu->OPHCT_byte;
	}

	if(addr == OPVCT)	
	{
		if(ppu->OPVCT_byte == 0)
		{
			uint8_t read = LE_LBYTE16(ppu->vscan_counter);
			mem_write(data_bus, addr, read);
			ppu->PPU2_bus = read;
		}
		else 
		{
			uint8_t read = LE_HBYTE16(ppu->vscan_counter);
			read = (read & 0x01) | (ppu->PPU2_bus & (~0x01));
			mem_write(data_bus, addr, read);
			ppu->PPU2_bus = read;
		}

		ppu->OPVCT_byte = ~ppu->OPVCT_byte;
	}

	if(addr == STAT77)
	{
		uint8_t stat = 0x00;

		if(ppu->time_over)
		{
			stat |= 0x80;
		}

		if(ppu->range_over)
		{
			stat |= 0x40;
		}

		if(ppu->pin_25)
		{
			stat |= 0x20;
		}

		stat |= ppu->PPU1_bus & 0x10;
		stat |= ppu->PPU1_version & 0b00001111;

		mem_write(data_bus, addr, stat);
		ppu->PPU1_bus = stat;
	}

	if(addr == STAT78)
	{
		uint8_t stat = 0x00;

		ppu->OPVCT_byte = 0;
		ppu->OPHCT_byte = 0;

		if(mem_read(data_bus, WRIO) & 0x80)
		{
			ppu->counter_latch = 0;
		}

		if(ppu->interlace_field)
		{
			stat |= 0x80;
		}

		if(ppu->counter_latch)
		{
			stat |= 0x40;
		}

		stat |= ppu->PPU2_bus & 0x20;

		if(ppu->frame_rate)
		{
			// 0 -> 60hz, 1 -> 50hz
			// will always be 60hz because in NTSC mode (525 lines)
			stat |= 0x10;
		}

		stat |= ppu->PPU2_version & 0b00001111;

		mem_write(data_bus, addr, stat);
		ppu->PPU2_bus = stat;
	}
}
