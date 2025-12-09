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

void write_ppu_register(struct PPU *ppu, struct PPU_memory *ppu_memory, struct Memory *memory, uint32_t addr, uint8_t write_value)
{
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
		if(!(ppu->OAM_addr & 0x0001))
		{
			ppu->OAM_latch = write_value;
		}
		else if(ppu->OAM_addr < OAM_LTABLE_BYTES)
		{
			ppu_memory->OAM_low_table[ppu->OAM_addr - 1] = ppu->OAM_latch;
			ppu_memory->OAM_low_table[ppu->OAM_addr] = write_value;
		}
		
		if(ppu->OAM_addr >= OAM_LTABLE_BYTES)
		{
			ppu_memory->OAM_high_table[ppu->OAM_addr] = write_value;
		}

		ppu->OAM_addr++;
	}

	if(addr == VMADDL)
	{
		// cannot read during blanks
		ppu->VRAM_addr = LE_COMBINE_2BYTE(mem_read(memory, VMADDL), mem_read(memory, VMADDH));
	}
	if(addr == VMADDH)
	{
		// cannot read during blanks
		ppu->VRAM_addr = LE_COMBINE_2BYTE(mem_read(memory, VMADDL), mem_read(memory, VMADDH));
	}
}

void read_ppu_register(struct PPU *ppu, struct PPU_memory *ppu_memory, struct Memory *memory, uint32_t addr)
{
	if(addr == OAMDATAREAD)
	{
		ppu->OAM_addr++;
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

uint8_t read_VRAM(struct PPU_memory *ppu_memory, uint16_t addr)
{
	return ppu_memory->VRAM[addr];
}

uint8_t read_CGRAM(struct PPU_memory *ppu_memory, uint16_t addr)
{
	return ppu_memory->VRAM[addr];
}

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
	uint8_t tile_size = ppu->BG1_character_size[0];
	int tile_x = (ppu->x + ppu->BG_scroll_offset.BGn_horizontal_offset[0]) / tile_size;
	int tile_y = (ppu->y + ppu->BG_scroll_offset.BGn_vertical_offset[0]) / tile_size;

	uint16_t BG1_addr = ppu->BGn_tilemap_info.tilemap_vram_addr[0] + (tile_y * horizontal_tilemaps) + tile_x;
	uint8_t BG1_addr_offset = ppu->BGn_chr_tiles_offset[0];

	struct tilemap BG1_tilemap;
	get_tilemap(&BG1_tilemap, ppu_memory, BG1_addr, BG1_addr_offset);

	uint16_t tilemap_entry = LE_COMBINE_2BYTE(read_VRAM(ppu_memory, BG1_tilemap.tile_addr), read_VRAM(ppu_memory, BG1_tilemap.tile_addr + 1));
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

void horizontal_scan(struct PPU *ppu, struct PPU_memory *ppu_memory, struct Memory *memory)
{
	ppu->x = 0;

	while(ppu->x < DOTS)
	{
		if(ppu->high_res)
		{
			ppu->x++;
		}
		else 
		{
			ppu->x += 2;
		}
	}
}

uint8_t BB_read_OAM()
{
	return 0x00;
}

void BB_write_OAM(uint8_t write_value)
{
}

uint8_t BB_read_CGRAM()
{
	return 0x00;
}

void BB_write_CGRAM(uint8_t write_value)
{
}

uint8_t BB_read_VRAM()
{
	return 0x00;
}

void BB_write_VRAM(uint8_t write_value)
{
}


