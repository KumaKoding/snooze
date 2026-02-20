#include "PPU.h"
#include "memory.h"
#include "ricoh5A22.h"
#include "registers.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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
	ppu->pixel_buf = malloc(DOTS * LINES * sizeof(Color_t));
	ppu->x = 0;
	ppu->y = 0;

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

	uint16_t screen_x = (ppu->y - HIDE_LINES) + ppu->BG_scroll_offset.BGn_horizontal_offset[layer];
	uint16_t screen_y = (ppu->x - HIDE_DOTS) + ppu->BG_scroll_offset.BGn_vertical_offset[layer];

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

		Color_t tiledata[64];

		for(int row = 0; row < 8; row++)
		{
			uint16_t bitplane = read_VRAM(data_bus, tile.tile_addr + row);

			for(uint8_t p = 0; p < 8; p++)
			{
				int palette_index = (check_bit16(bitplane, 0x10 << p) << 1) | check_bit16(bitplane, 0x01 << p);
				uint16_t color_data = read_CGRAM(data_bus, palette_index + (tile.palette * 4));

				rgba_from_CGRAM(&tiledata[(row * 8) + p], color_data);

				if(palette_index == 0)
				{
					tiledata[(row * 8) + p].a = 0;
				}
				else 
				{
					tiledata[(row * 8) + p].a = 0xFF;
				}
			}
		}

		int tile_x = screen_x % 8;
		int tile_y = screen_y % 8;

		ppu->pixel_buf[(screen_y * DOTS) + screen_x] = tiledata[(tile_y * 8) + tile_x];
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
		//
		// uint16_t bitplane1 = read_VRAM(data_bus, tile.tile_addr);
		// uint16_t bitplane2 = read_VRAM(data_bus, tile.tile_addr + 1);
		// uint16_t bitplane3 = read_VRAM(data_bus, tile.tile_addr + TILESET_ROW_SIZE);
		// uint16_t bitplane4 = read_VRAM(data_bus, tile.tile_addr + TILESET_ROW_SIZE + 1);
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

#define exit_hblank(x) (x == HIDE_DOTS)
#define enter_hblank(x) (x == VISIBLE_DOTS + HIDE_DOTS)
#define exit_line(x) (x >= DOTS)

#define exit_vblank(y) (y == HIDE_LINES)
#define enter_vblank(y) (y == VISIBLE_LINES + HIDE_LINES)
#define exit_screen(y) (y >= LINES)

void move_beam(struct data_bus *data_bus)
{
	struct PPU *ppu = data_bus->B_bus.ppu->ppu;

	ppu->x++;

	if(exit_hblank(ppu->x))
	{
		clear_hblank(data_bus);
	}

	if(enter_hblank(ppu->x))
	{
		signal_hblank(data_bus);

		ppu->queued_cycles += 68;
	}

	if(exit_line(ppu->x))
	{
		ppu->y++;
		ppu->x = 0;
	}

	if(enter_vblank(ppu->y))
	{
		signal_vblank(data_bus);
	}

	if(exit_vblank(ppu->y))
	{
		clear_vblank(data_bus);

		ppu->range_over = 0;
		ppu->time_over = 0;

		ppu->interlace_field = ~ppu->interlace_field;
	}

	if(exit_screen(ppu->y))
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

	if(ppu->BG_mode == 0)
	{
		M0_dot(data_bus);
	}

	check_IRQ(data_bus);
	move_beam(data_bus);
}

