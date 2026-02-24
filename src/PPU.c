#include "PPU.h"
#include "memory.h"
#include "utility.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

void init_ppu(struct PPU *ppu)
{
	ppu->pixel_buf = malloc(DOTS * LINES * sizeof(uint8_t) * 3);
	ppu->x = 0;
	ppu->y = 0;

	ppu->active_scan = 0;
	ppu->active_x = 0;
	ppu->active_y = 0;

	ppu->queued_cycles = 0;
}

void init_ppu_memory(struct PPU_memory *ppu_memory)
{
	ppu_memory->VRAM = malloc(VRAM_WORD_WIDTH * 2);
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

void get_tile(struct tilemap *tilemap, struct data_bus *data_bus, uint16_t VRAM_addr, uint8_t VRAM_page_offset)
{
	uint16_t tilemap_data = read_VRAM(data_bus, VRAM_addr);

	tilemap->flip_vertical = check_bit16(tilemap_data, 0x8000);
	tilemap->flip_horizontal = check_bit16(tilemap_data, 0x4000);
	tilemap->priority = check_bit16(tilemap_data, 0x2000);
	tilemap->palette = (tilemap_data & 0b0001110000000000) >> 10;
	tilemap->tile_addr = (0x0000 | VRAM_page_offset) << 12;
	tilemap->tile_addr = tilemap->tile_addr | (tilemap_data & 0b0000001111111111);
}

void rgba_from_CGRAM(Color_t *color, uint16_t cg)
{
	const int BITS_PER_CHANNEL= 5;

	uint8_t r = cg & 0b11111;
	r = ((float)r / 0b11111) * 0xFF;
	uint8_t g = (cg >> BITS_PER_CHANNEL) & 0b11111;
	g = ((float)g / 0b11111) * 0xFF;
	uint8_t b = (cg >> (BITS_PER_CHANNEL * 2)) & 0b11111;
	b = ((float)b / 0b11111) * 0xFF;

	color->r = r;
	color->g = g;
	color->b = b;
}

void M0_dot(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;
	int layer = 0;

	// int horizontal_tiles = ppu->BGn_tilemap_info.horizontal_tilemaps[layer];
	int vertical_tiles = ppu->BGn_tilemap_info.vertical_tilemaps[layer];

	uint16_t VRAM_addr = ppu->BGn_tilemap_info.tilemap_vram_addr[layer] << 10;

	uint16_t screen_x = ppu->active_x + ppu->BG_scroll_offset.BGn_horizontal_offset[layer];
	uint16_t screen_y = ppu->active_x + ppu->BG_scroll_offset.BGn_vertical_offset[layer];

	if(ppu->BGn_character_size[layer] == CH_SIZE_8x8)
	{
		int scaled_x = screen_x / 8;
		int scaled_y = screen_y / 8;

		if(scaled_x > TILEMAP_BASE_SIDE)
		{
			scaled_x = (scaled_x - TILEMAP_BASE_SIDE);
			scaled_x += (TILEMAP_BASE_SIDE) * (vertical_tiles * TILEMAP_BASE_SIDE);
		}

		VRAM_addr += scaled_x;
		VRAM_addr += scaled_y * TILEMAP_BASE_SIDE;

		struct tilemap tile;
		get_tile(&tile, data_bus, VRAM_addr, ppu->BGn_chr_tiles_offset[layer]);

		int tile_x = screen_x % 8;
		int tile_y = screen_y % 8;

		uint16_t bitplane = read_VRAM(data_bus, tile.tile_addr + tile_y);
		int hi_bit = check_bit16(bitplane, 0x0100 << tile_x);
		int lo_bit = check_bit16(bitplane, 0x0001 << tile_x);
		int index = ((0 | hi_bit) << 1) | lo_bit;

		uint16_t CGRAM_addr = (tile.palette * 4) + index;
		
		Color_t pixel;
		rgba_from_CGRAM(&pixel, read_CGRAM(data_bus, CGRAM_addr));

		if(pixel.a)
		{
			int index = ((ppu->active_y * VISIBLE_DOTS) + ppu->active_x) * 3;
			ppu->pixel_buf[index] = pixel.r;
			ppu->pixel_buf[index + 1] = pixel.g;
			ppu->pixel_buf[index + 2] = pixel.b;
		}
	}
	else if(ppu->BGn_character_size[layer] == CH_SIZE_16x16)
	{
		int scaled_x = screen_x / 16;
		int scaled_y = screen_y / 16;

		if(scaled_x > TILEMAP_BASE_SIDE)
		{
			scaled_x = (scaled_x - TILEMAP_BASE_SIDE);
			scaled_x += (TILEMAP_BASE_SIDE) * (vertical_tiles * TILEMAP_BASE_SIDE);
		}

		VRAM_addr += scaled_x;
		VRAM_addr += scaled_y * TILEMAP_BASE_SIDE;

		struct tilemap tile;
		get_tile(&tile, data_bus, VRAM_addr, ppu->BGn_chr_tiles_offset[layer]);

		int tile_x = screen_x % 16;
		int tile_y = screen_y % 16;

		uint16_t adjusted_tile_addr = tile.tile_addr;

		if(tile_x >= 8)
		{
			adjusted_tile_addr++;
		}

		if(tile_y >= 8)
		{
			adjusted_tile_addr += TILESET_ROW_SIZE;
		}

		uint16_t bitplane = read_VRAM(data_bus, adjusted_tile_addr + tile_y);
		int hi_bit = check_bit16(bitplane, 0x0100 << tile_x);
		int lo_bit = check_bit16(bitplane, 0x0001 << tile_x);
		int index = ((0 | hi_bit) << 1) | lo_bit;

		uint16_t CGRAM_addr = (tile.palette * 4) + index;

		Color_t pixel;
		rgba_from_CGRAM(&pixel, read_CGRAM(data_bus, CGRAM_addr));

		if(pixel.a)
		{
			int index = ((ppu->active_y * VISIBLE_DOTS) + ppu->active_x) * 3;
			ppu->pixel_buf[index] = pixel.r;
			ppu->pixel_buf[index + 1] = pixel.g;
			ppu->pixel_buf[index + 2] = pixel.b;
		}
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

int enter_hblank(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(ppu->x == HIDE_DOTS + VISIBLE_DOTS)
	{
		return 1;
	}

	return 0;
}

int enter_vblank(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(ppu->x == 0 && ppu->y == 0xF0 && ppu->overscan)
	{
		return 1;
	}

	if(ppu->x == 0 && ppu->y == 0xE1 && !ppu->overscan)
	{
		return 1;
	}

	return 0;
}

int exit_vblank(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(ppu->x == 0 && ppu->y == 0)
	{
		return 1;
	}

	return 0;
}

int exit_hblank(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(ppu->x == HIDE_DOTS)
	{
		return 1;
	}

	return 0;
}

int exit_screen(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(ppu->y >= LINES)
	{
		return 1;
	}

	return 0;
}

int exit_line(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(ppu->x >= DOTS)
	{
		return 1;
	}

	return 0;
}

void move_beam(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	ppu->x++;

	if(ppu->active_scan)
	{
		ppu->active_x++;
	}

	if(exit_hblank(data_bus))
	{
		clear_hblank(data_bus);

		ppu->active_scan = 1;
		ppu->active_x = 0;
		ppu->active_y++;
	}

	if(enter_hblank(data_bus))
	{
		signal_hblank(data_bus);

		ppu->queued_cycles += 68;
		ppu->active_scan = 0;
	}

	if(exit_line(data_bus))
	{
		ppu->y++;
		ppu->x = 0;
	}

	if(enter_vblank(data_bus))
	{
		signal_vblank(data_bus);
	}

	if(exit_vblank(data_bus))
	{
		clear_vblank(data_bus);

		ppu->range_over = 0;
		ppu->time_over = 0;

		ppu->interlace_field = ~ppu->interlace_field;

		ppu->active_y = 0;
	}

	if(exit_screen(data_bus))
	{
		ppu->y = 0;
	}
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

	if(ppu->x == 536 / 4)
	{
		set_refresh(data_bus);
	}

	if(ppu->BG_mode == 0)
	{
		M0_dot(data_bus);
	}

	check_IRQ(data_bus);
	move_beam(data_bus);
}

