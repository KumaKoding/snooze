#include "PPU.h"
#include "memory.h"
#include "utility.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

void init_ppu(struct PPU *ppu)
{
	ppu->x = 0;
	ppu->y = 0;

	ppu->active_scan = 0;
	ppu->active_x = 0;
	ppu->active_y = 0;

	ppu->queued_cycles = 0;

	ppu->frame_finished = 0;
}

void init_ppu_memory(struct PPU_memory *ppu_memory)
{
	ppu_memory->VRAM = malloc(VRAM_WORDS * VRAM_WORD_WIDTH);
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

	// printf("%04x: %d, %04x\n", VRAM_addr, tilemap->palette, tilemap->tile_addr);
}

void rgba_from_CGRAM(Color_t *color, uint16_t cg)
{
	const int BITS_PER_CHANNEL = 5;
	const int RGB24_SHIFT = 3;

	uint8_t r = cg & 0b11111;
	uint8_t g = (cg >> BITS_PER_CHANNEL) & 0b11111;
	uint8_t b = (cg >> (BITS_PER_CHANNEL * 2)) & 0b11111;

	g = g << RGB24_SHIFT;
	r = r << RGB24_SHIFT;
	b = b << RGB24_SHIFT;

	color->r = r;
	color->g = g;
	color->b = b;
	color->a = 0xFF;
}

void M0_dot(struct data_bus *data_bus, SDL_Surface *frame_buffer)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;
	int layer = 0;

	// int horizontal_tiles = ppu->BGn_tilemap_info.horizontal_tilemaps[layer];
	int vertical_tiles = ppu->BGn_tilemap_info.vertical_tilemaps[layer];

	uint16_t VRAM_addr = (ppu->BGn_tilemap_info.tilemap_vram_addr[layer] << 10) & (~0x8000);
	printf("%02x -> %04x -> ", ppu->BGn_tilemap_info.tilemap_vram_addr[layer], VRAM_addr);

	uint16_t screen_x = (ppu->active_x / 2) + ppu->BG_scroll_offset.BGn_horizontal_offset[layer];
	uint16_t screen_y = (ppu->active_y / 2) + ppu->BG_scroll_offset.BGn_vertical_offset[layer];

	if(ppu->active_x == 0)
	{
		printf("adflkdjadf\n");
	}

	if(ppu->BGn_character_size[layer] == CH_SIZE_8x8)
	{
		int scaled_x = screen_x / 8; // because we're counting double width for interlacing/high res
		int scaled_y = screen_y / 8;

		if(scaled_x > TILEMAP_BASE_SIDE)
		{
			printf("alfkjsdlfa\n");
			scaled_x = (scaled_x - TILEMAP_BASE_SIDE);
			scaled_x += (TILEMAP_BASE_SIDE) * (vertical_tiles * TILEMAP_BASE_SIDE);
			// printf("%d %d\n", scaled_x, scaled_y);
		}


		VRAM_addr += scaled_x;
		VRAM_addr += scaled_y * TILEMAP_BASE_SIDE;

		struct tilemap tile;
		get_tile(&tile, data_bus, VRAM_addr, ppu->BGn_chr_tiles_offset[layer]);


		printf("%04x -> %04x -> ", VRAM_addr, tile.tile_addr);

		int tile_x = screen_x % 8;
		int tile_y = screen_y % 8;

		uint16_t bitplane = read_VRAM(data_bus, (tile.tile_addr * 8) + tile_y);
		printf("%04x\n", bitplane);
		int hi_bit = check_bit16(bitplane, 0x8000 >> tile_x);
		int lo_bit = check_bit16(bitplane, 0x0080 >> tile_x);
		int index = ((0 | hi_bit) << 1) | lo_bit;

		uint16_t CGRAM_addr = (tile.palette * 4) + index;
		// the VRAM malloc() was too small, so there was always a heap buffer overflow which corrupted the other stuff causing invalid addresses

		Color_t pixel;
		rgba_from_CGRAM(&pixel, read_CGRAM(data_bus, CGRAM_addr));

		SDL_WriteSurfacePixel(frame_buffer, ppu->x, ppu->y, pixel.r, pixel.g, pixel.b, 255);
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
		printf("%d\n", index);

		uint16_t CGRAM_addr = (tile.palette * 4) + index;

		//somethings messing with the actual pointer to the ppu

		Color_t pixel;
		rgba_from_CGRAM(&pixel, read_CGRAM(data_bus, CGRAM_addr));

		if(pixel.a)
		{
			// int index = ((ppu->active_y * VISIBLE_DOTS) + ppu->active_x) * 3;
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

	if(ppu->x == 0 && ppu->y == 0xF0 * 2 && ppu->overscan)
	{
		return 1;
	}

	if(ppu->x == 0 && ppu->y == 0xE1 * 2 && !ppu->overscan)
	{
		return 1;
	}

	return 0;
}

int exit_vblank(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if(ppu->x >= DOTS && ppu->y >= LINES)
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

	if(enter_vblank(data_bus))
	{
		signal_vblank(data_bus);

		ppu->frame_finished = 1;
	}

	if(enter_hblank(data_bus))
	{
		signal_hblank(data_bus);
		allow_HDMA(data_bus);

		ppu->queued_cycles += 68;
	}

	if(exit_vblank(data_bus))
	{
		clear_vblank(data_bus);

		ppu->range_over = 0;
		ppu->time_over = 0;

		ppu->interlace_field = ~ppu->interlace_field;

		ppu->active_y = 0;
	}

	if(exit_hblank(data_bus))
	{
		clear_hblank(data_bus);
		disallow_HDMA(data_bus);

		ppu->active_x = 0;
		ppu->active_y += 2;
	}

	if(exit_line(data_bus))
	{
		if(exit_screen(data_bus))
		{
			ppu->y = 0;
		}
		else 
		{
			ppu->y += 2;
		}

		ppu->x = 0;
	}
	else 
	{	
		ppu->x += 2;

		if(ppu->active_scan)
		{
			ppu->active_x += 2;
		}
	}

	ppu->active_scan = 0;

	if(HIDE_DOTS <= ppu->x && ppu->x < VISIBLE_DOTS + HIDE_DOTS)
	{
		if(HIDE_LINES <= ppu->y && ppu->y < VISIBLE_LINES + HIDE_LINES)
		{
			ppu->active_scan = 1;
		}
	}
}

void ppu_dot(struct data_bus *data_bus, SDL_Surface *frame_buffer)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	if((ppu->x == 322 || ppu->x == 326) && ppu->y != 239)
	{
		long_dot(data_bus);
	}
	else 
	{
		// takes 4 times as long because there are double x / y
		short_dot(data_bus);
	}

	if(ppu->x == 536 / 4)
	{
		set_refresh(data_bus);
	}

	if(ppu->BG_mode == 0 && ppu->active_scan && ppu->active_y % 2 == 0 && ppu->active_x % 2 ==0)
	{
		M0_dot(data_bus, frame_buffer);
	}


	check_IRQ(data_bus);
	move_beam(data_bus);
}

