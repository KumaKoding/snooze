#include "PPU.h"
#include "memory.h"
#include <stdint.h>

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

uint16_t read_VRAM(struct PPU_memory *ppu_memory, uint16_t addr)
{
	return LE_COMBINE_2BYTE(ppu_memory->VRAM[addr * 2], ppu_memory->VRAM[addr * 2 + 1]);
}

void write_VRAM_word(struct PPU_memory *ppu_memory, uint16_t addr, uint16_t word)
{
	ppu_memory->VRAM[addr * 2] = LE_LBYTE16(word);
	ppu_memory->VRAM[addr * 2 + 1] = LE_HBYTE16(word);
}

void write_VRAM_low(struct PPU_memory *ppu_memory, uint16_t addr, uint8_t byte)
{
	ppu_memory->VRAM[addr * 2] = byte;
}

void write_VRAM_high(struct PPU_memory *ppu_memory, uint16_t addr, uint8_t byte)
{
	ppu_memory->VRAM[addr * 2 + 1] = byte;
}

uint8_t read_OAM(struct PPU_memory *ppu_memory, uint16_t addr)
{
	if(check_bit16(addr, 0x0200))
	{
		return ppu_memory->OAM_high_table[addr & 0x01FF];
	}
	else 
	{
		return ppu_memory->OAM_low_table[addr & 0x01FF];
	}
}

void write_OAM(struct PPU_memory *ppu_memory, uint16_t addr, uint8_t byte)
{
	if(check_bit16(addr, 0x0200))
	{
		ppu_memory->OAM_high_table[addr & 0x01FF] = byte;
	}
	else 
	{
		ppu_memory->OAM_low_table[addr & 0x01FF] = byte;
	}
}

void write_CGRAM_word(struct PPU_memory *ppu_memory, uint16_t addr, uint16_t word)
{
	ppu_memory->CGRAM[addr * 2] = LE_LBYTE16(word);
	ppu_memory->CGRAM[addr * 2 + 1] = LE_HBYTE16(word);
}

uint16_t read_CGRAM(struct PPU_memory *ppu_memory, uint16_t addr)
{
	return LE_COMBINE_2BYTE(ppu_memory->CGRAM[addr * 2], ppu_memory->CGRAM[addr * 2 + 1]);
}

// TODO
//
// OPHCT
// OPVCT
// STAT77
// STAT78

void write_ppu_register(struct PPU *ppu, struct PPU_memory *ppu_memory, struct Memory *memory, uint32_t addr, uint8_t write_value)
{
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
		ppu->OAM_addr = LE_COMBINE_2BYTE(mem_read(memory, OAMADDL), mem_read(memory, OAMADDH)) * 2;
	}

	if(addr == OAMADDH)
	{
		ppu->OAM_addr = LE_COMBINE_2BYTE(mem_read(memory, OAMADDL), mem_read(memory, OAMADDH)) * 2;
	}

	if(addr == OAMDATA)
	{
		if(!check_bit16(ppu->OAM_addr, 0x0001))
		{
			ppu->OAM_latch = write_value;
		}
		else if(ppu->OAM_addr < OAM_LTABLE_BYTES)
		{
			write_OAM(ppu_memory, ppu->OAM_addr - 1, ppu->OAM_latch);
			write_OAM(ppu_memory, ppu->OAM_addr, write_value);
		}
		
		if(ppu->OAM_addr >= OAM_LTABLE_BYTES)
		{
			write_OAM(ppu_memory, ppu->OAM_addr, write_value);
		}

		ppu->OAM_addr++;
	}

	if(addr == VMADDL)
	{
		// cannot read during blanks
		ppu->VRAM_addr = LE_COMBINE_2BYTE(mem_read(memory, VMADDL), mem_read(memory, VMADDH)) * 2;
		ppu->VRAM_latch = read_VRAM(ppu_memory, ppu->VRAM_addr);
	}

	if(addr == VMADDH)
	{
		// cannot read during blanks
		ppu->VRAM_addr = LE_COMBINE_2BYTE(mem_read(memory, VMADDL), mem_read(memory, VMADDH));
		ppu->VRAM_latch = read_VRAM(ppu_memory, ppu->VRAM_addr);
	}

	if(addr == VMDATAL)
	{
		write_VRAM_low(ppu_memory, ppu->VRAM_addr, write_value);

		if(ppu->VRAM_increment_mode == 0)
		{
			ppu->VRAM_addr++;
		}
	}

	if(addr == VMDATAH)
	{
		write_VRAM_high(ppu_memory, ppu->VRAM_addr, write_value);

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
			write_CGRAM_word(ppu_memory, ppu->CGRAM_addr, LE_COMBINE_2BYTE(ppu->CGRAM_latch, write_value));
			ppu->CGRAM_check = 0;
		}
	}
}

void read_ppu_register(struct PPU *ppu, struct PPU_memory *ppu_memory, struct Memory *memory, uint32_t addr)
{
	if(addr == OAMDATAREAD)
	{
		mem_write(memory, OAMDATAREAD, read_OAM(ppu_memory, ppu->OAM_addr));

		ppu->OAM_addr++;
	}

	if(addr == VMDATALREAD)
	{
		mem_write(memory, VMDATALREAD, LE_LBYTE16(ppu->VRAM_latch));

		if(ppu->VRAM_increment_mode == 0)
		{
			ppu->VRAM_addr++;
		}
	}

	if(addr == VMDATAHREAD)
	{
		mem_write(memory, VMDATAHREAD, LE_HBYTE16(ppu->VRAM_latch));

		if(ppu->VRAM_increment_mode == 1)
		{
			ppu->VRAM_addr++;
		}
	}

	if(addr == CGDATAREAD)
	{
		if(ppu->CGRAM_check)
		{
			mem_write(memory, CGDATAREAD, LE_HBYTE16(read_CGRAM(ppu_memory, ppu->CGRAM_addr)));
			ppu->CGRAM_check = 0;
		}
		else
		{
			mem_write(memory, CGDATAREAD, LE_LBYTE16(read_CGRAM(ppu_memory, ppu->CGRAM_addr)));
			ppu->CGRAM_check = 1;
		}
	}

	if(addr == SLHV)
	{
		ppu->HV_latch = 1;

		ppu->hscan_counter = ppu->x / 2;
		ppu->vscan_counter = ppu->y / 2;
	}
}

struct tilemap 
{
	int flip_vertical;
	int flip_horizontal;
	int priority;
	uint8_t palette;
	uint16_t tile_addr;
};

void get_tilemap(struct tilemap *tilemap, struct PPU_memory *ppu_memory, uint16_t VRAM_addr, uint8_t VRAM_page_offset)
{
	uint16_t tilemap_data = LE_COMBINE_2BYTE(read_VRAM(ppu_memory, VRAM_addr), read_VRAM(ppu_memory, VRAM_addr + 1));

	tilemap->flip_vertical = check_bit16(tilemap_data, 0x8000);
	tilemap->flip_horizontal = check_bit16(tilemap_data, 0x4000);
	tilemap->priority = check_bit16(tilemap_data, 0x2000);
	tilemap->palette = (tilemap_data & 0b0001110000000000) >> 10;
	tilemap->tile_addr = (0x0000 | VRAM_page_offset) << 13;
	tilemap->tile_addr = tilemap->tile_addr | (tilemap_data & 0b0000001111111111);
}

void M0_dot(struct PPU *ppu, struct PPU_memory *ppu_memory, struct Memory *memory)
{
	uint8_t horizontal_tilemaps = ppu->BGn_tilemap_info.horizontal_tilemaps[0];
	uint8_t tile_size = ppu->BGn_character_size[0];
	int tile_x = (ppu->x + ppu->BG_scroll_offset.BGn_horizontal_offset[0]) / tile_size;
	int tile_y = (ppu->y + ppu->BG_scroll_offset.BGn_vertical_offset[0]) / tile_size;

	uint16_t BG1_addr = ppu->BGn_tilemap_info.tilemap_vram_addr[0] + (tile_y * horizontal_tilemaps) + tile_x;
	uint8_t BG1_addr_offset = ppu->BGn_chr_tiles_offset[0];

	struct tilemap BG1_tilemap;
	get_tilemap(&BG1_tilemap, ppu_memory, BG1_addr, BG1_addr_offset);

	uint16_t tilemap_entry = read_VRAM(ppu_memory, BG1_tilemap.tile_addr);
	int bitplane_index = ppu->x % 8;
	uint8_t color_bits = (LE_LBYTE16(tilemap_entry) << bitplane_index) & 0x01;
	color_bits |= (LE_HBYTE16(tilemap_entry) << (bitplane_index - 1)) & 0x02;

	uint16_t color = read_CGRAM(ppu_memory, (BG1_tilemap.palette * 4) + color_bits);

	ppu->pixel_buf[(ppu->y * DOTS * 2) + (ppu->x * 3 * 2)] = (color & 0b0111110000000000) >> 10;
	ppu->pixel_buf[(ppu->y * DOTS * 2) + (ppu->x * 3 * 2) + 1] = color & 0b0000001111100000;
	ppu->pixel_buf[(ppu->y * DOTS * 2) + (ppu->x * 3 * 2) + 2] = color & 0b0000000000011111;

	if(ppu->x >= DOTS)
	{
		ppu->x = 0;
		ppu->y++;
	}

	if(ppu->y >= LINES)
	{
		ppu->y = 0;

		mem_write(memory, NMI, 0x80);
	}
}

