#include "PPU.h"
#include "memory.h"
#include "registers.h"
#include <stdint.h>
#include <stdlib.h>

#define LE_HBYTE16(u16) (uint8_t)((u16 & 0xFF00) >> 8)
#define LE_LBYTE16(u16) (uint8_t)(u16 & 0x00FF)

#define SWP_LE_LBYTE16(u16, u8) ((u16 & 0xFF00) | u8)
#define SWP_LE_HBYTE16(u16, u8) ((u16 & 0x00FF) | ((0x0000 | u8) << 8))
#define LE_COMBINE_2BYTE(b1, b2) (uint16_t)(((0x0000 | b2) << 8) | b1)

// static uint8_t check_bit8(uint8_t ps, uint8_t mask)
// {
// 	if((ps & mask) == mask)
// 	{
// 		return 0x01;
// 	}
//
// 	return 0x00;
// }

static uint8_t check_bit16(uint16_t ps, uint16_t mask)
{
	if((ps & mask) == mask)
	{
		return 0x01;
	}

	return 0x00;
}

void init_ppu(struct PPU *ppu)
{
	ppu->pixel_buf = malloc(DOTS * LINES * 3);
	ppu->x = 0;
	ppu->y = 0;

	ppu->queued_cycles = 0;
	ppu->elapsed_cycles = 0;
}

void init_ppu_memory(struct PPU_memory *ppu_memory)
{
	ppu_memory->VRAM= malloc(VRAM_WORD_WIDTH * 2);
	ppu_memory->OAM_high_table = malloc(OAM_HTABLE_BYTES);
	ppu_memory->OAM_low_table = malloc(OAM_LTABLE_BYTES);
	ppu_memory->CGRAM = malloc(CGRAM_WORDS * 2);
}

void init_s_ppu(struct S_PPU *s_ppu)
{
	s_ppu->ppu = malloc(sizeof(struct PPU));
	s_ppu->memory = malloc(sizeof(struct PPU_memory));

	init_ppu(s_ppu->ppu);
	init_ppu_memory(s_ppu->memory);
}

struct tilemap 
{
	int flip_vertical;
	int flip_horizontal;
	int priority;
	uint8_t palette;
	uint16_t tile_addr;
};

void get_tilemap(struct tilemap *tilemap, struct data_bus *data_bus, uint16_t VRAM_addr, uint8_t VRAM_page_offset)
{
	uint16_t tilemap_data = LE_COMBINE_2BYTE(read_VRAM(data_bus, VRAM_addr), read_VRAM(data_bus, VRAM_addr + 1));

	tilemap->flip_vertical = check_bit16(tilemap_data, 0x8000);
	tilemap->flip_horizontal = check_bit16(tilemap_data, 0x4000);
	tilemap->priority = check_bit16(tilemap_data, 0x2000);
	tilemap->palette = (tilemap_data & 0b0001110000000000) >> 10;
	tilemap->tile_addr = (0x0000 | VRAM_page_offset) << 13;
	tilemap->tile_addr = tilemap->tile_addr | (tilemap_data & 0b0000001111111111);
}

void M0_dot(struct data_bus *data_bus, struct Memory *memory)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	uint8_t horizontal_tilemaps = ppu->BGn_tilemap_info.horizontal_tilemaps[0];
	uint8_t tile_size = ppu->BGn_character_size[0];
	int tile_x = (ppu->x + ppu->BG_scroll_offset.BGn_horizontal_offset[0]) / tile_size;
	int tile_y = (ppu->y + ppu->BG_scroll_offset.BGn_vertical_offset[0]) / tile_size;

	uint16_t BG1_addr = ppu->BGn_tilemap_info.tilemap_vram_addr[0] + (tile_y * horizontal_tilemaps) + tile_x;
	uint8_t BG1_addr_offset = ppu->BGn_chr_tiles_offset[0];

	struct tilemap BG1_tilemap;
	get_tilemap(&BG1_tilemap, data_bus, BG1_addr, BG1_addr_offset);

	uint16_t tilemap_entry = read_VRAM(data_bus, BG1_tilemap.tile_addr);
	int bitplane_index = ppu->x % 8;
	uint8_t color_bits = (LE_LBYTE16(tilemap_entry) << bitplane_index) & 0x01;
	color_bits |= (LE_HBYTE16(tilemap_entry) << (bitplane_index - 1)) & 0x02;

	uint16_t color = read_CGRAM(data_bus, (BG1_tilemap.palette * 4) + color_bits);

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
		ppu->time_over = 0;
		ppu->range_over = 0;
		ppu->interlace_field = ~ppu->interlace_field;
		ppu->y = 0;

		write_register_raw(data_bus, RDNMI, 0x80);
	}
}

void long_dot(struct data_bus *data_bus)
{
	data_bus->B_bus.ppu->ppu->queued_cycles += 6;
}

void short_dot(struct data_bus *data_bus)
{
	data_bus->B_bus.ppu->ppu->queued_cycles += 4;
}


void ppu_dot(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if((ppu->x == 322 || ppu->x == 326) && ppu->y != 239)
	{
		long_dot(data_bus);
	}
	else 
	{
		short_dot(data_bus);
	}

	ppu->x++;

	if(ppu->x >= DOTS - 1)
	{
		ppu->y++;
		ppu->x = 0;
	}

	if(ppu->y >= LINES)
	{
		ppu->y = 0;
	}
}

