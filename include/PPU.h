#ifndef PPU_H
#define PPU_H

#include "memory.h"
#include <stdint.h>

#define DOTS 640
#define LINES 525

#define HIDE_DOTS 44
#define HIDE_LINES 2

#define VISIBLE_DOTS 512
#define VISIBLE_LINES 448

#define TILEMAP_BASE_SIDE 32

#define M0_BACKGROUNDS 4

#define VRAM_WORD_WIDTH 2
#define VRAM_WORDS 32 * 1024
#define OAM_LTABLE_BYTES 512 
#define OAM_HTABLE_BYTES 32
#define CGRAM_WORDS 256

struct PPU_memory
{
	uint8_t *VRAM;
	uint8_t *OAM_low_table;
	uint8_t *OAM_high_table;
	uint8_t *CGRAM;
};

struct S_PPU
{
	struct PPU *ppu;
	struct PPU_memory *memory;
};

enum sprite_sizes
{
	S_8x8,
	S_16x16,
	S_32x32,
	S_64x64,
	S_16x32,
	S_32x64
};


enum window_mask
{
	OR_mask,
	AND_mask,
	XOR_mask,
	XNOR_mask
};

enum window_regions
{
	Nowhere,
	Outside,
	Inside,
	Everywhere
};

struct PPU
{
	uint8_t *pixel_buf;

	int F_blank;
	uint8_t brightness;
	enum sprite_sizes obj_sizes[2];
	uint8_t name_select;
	uint8_t name_base_addr;
	uint16_t OAM_addr; // 15-bit, bit 15 = priority rotation
	uint8_t OAM_write;
	uint8_t OAM_latch;
	uint8_t BG_mode;
	int M1_BG3_priority;
	enum 
	{
		CH_SIZE_8x8,
		CH_SIZE_16x16
	} BGn_character_size[4]; // 0 -> 8x8, 1 -> 16x16
	uint8_t mosaic_size;
	int BGn_mosaic_enable[4];
	struct
	{
		uint8_t tilemap_vram_addr[4];
		uint8_t horizontal_tilemaps[4]; // 0 -> 1 tilemap, 1 -> 2 tilemaps;
		uint8_t vertical_tilemaps[4]; // 0 -> 1 tilemap, 1 -> 2 tilemaps;
	} BGn_tilemap_info;
	uint8_t BGn_chr_tiles_offset[4];
	struct
	{
		uint16_t BGn_horizontal_offset[4];
		uint16_t BGn_vertical_offset[4];
		uint8_t BG_offset_latch;
		uint8_t PPU2_horizontal_latch; // I don't know if it's right, but is what made the most sense to me https://forums.nesdev.org/viewtopic.php?p=184522
									   // In case the server goes down:
									   //	AWJ: "I'm almost 100% certain that the bits of the BGnHOFS 
									   //		  registers are actually split between the two chips that 
									   //		  comprise the S-PPU: the low 3 bits are on PPU2, and the 
									   //		  rest of the bits are on PPU1."
									   //	lidnariq: "Oh, that would make the weird bit math much clearer.
									   // 
									   //			   It would mean the situation is something more nearly like:
									   //			   new_coarse_nHOFS = (cur_write<<5) | (PPU1_HVOFS_LATCH>>3)
									   //			   new_fine_nHOFS = PPU2_nHOFS_LATCH"
									   // So, in theory, bits 0-2 control fine scroll, and the rest are coarse scroll, but are combined into 1
	} BG_scroll_offset;	
	int VRAM_increment_mode; // 0 -> After 2118, 2139, 1 -> After 2119, 213A
	enum 
	{
		None,
		M_2bpp,
		M_4bpp,
		M_8bpp
	} VRAM_remap;
	uint8_t address_increment;
	uint16_t VRAM_addr;
	uint16_t VRAM_latch;
	uint16_t VRAM_write;
	int tilemap_repeat; // 0 -> repeat, 1 -> no-repeat
	int non_tilemap_fill; // 0 -> transparent, 1 -> character 0
	int flip_BG_vertical;
	int flip_BG_horizontal;
	struct
	{
		uint16_t A;
		int16_t signed_16bit_mult;

		uint16_t B;
		int8_t signed_8bit_mult;

		uint16_t C;
		uint16_t D;

		uint16_t center_X;
		uint16_t center_Y;
	} M7_matrices;
	uint8_t CGRAM_addr;
	uint8_t CGRAM_write;
	uint8_t CGRAM_check;
	uint8_t CGRAM_latch;
	struct 
	{
		int invert_BGn_window_1[4];
		int invert_OBJ_window_1;
		int invert_BGn_window_2[4];
		int invert_OBJ_window_2;

		int enable_BGn_window_1[4];
		int enable_OBJ_window_1;
		int enable_BGn_window_2[4];
		int enable_OBJ_window_2;
	} window_select;
	struct 
	{
		uint8_t W1_left;
		uint8_t W1_right;

		uint8_t W2_left;
		uint8_t W2_right;
	} window_position;
	enum window_mask BGn_window_mask[4];
	enum window_mask OBJ_window_mask;
	enum window_mask Color_window_mask;
	int OBJ_main_enable;
	int BGn_main_enable[4];
	int OBJ_sub_enable;
	int BGn_sub_enable[4];
	int apply_window_OBJ_main;
	int apply_window_BGn_main[4];
	enum window_regions subscreen_transparent_region;
	enum window_regions mainscreen_black_region;
	int addend; // 0 -> fixed color, 1 -> subscreen
	int direct_color;
	int operator; // 0 -> add, 1 -> subtract
	int half_color_math;
	int backdrop_color_math;
	int obj_color_math;
	int BGn_color_math[4];
	uint8_t fixed_blue;
	uint8_t fixed_green;
	uint8_t fixed_red;
	int external_image_sync;
	int M7_EXTBG;
	int high_res;
	int overscan;
	int OBJ_interlacing;
	int screen_interlacing;
	uint32_t multiplication_result;
	uint8_t HV_latch; // when you read, you have to update OPVCT
	uint8_t OAM_read;
	uint16_t VRAM_read;
	uint8_t CGRAM_read;
	uint16_t vscan_counter; // read twice
	uint16_t hscan_counter; // read twice
	uint8_t OPVCT_byte;
	uint8_t OPHCT_byte;
	int time_over;
	int range_over;
	int pin_25;
	int PPU1_open_bus;
	uint8_t PPU1_version;
	int interlace_field;
	int counter_latch; 
	int PPU2_open_bus;
	int frame_rate; // 0 -> 60hz, 1 -> 50hz
	uint8_t PPU2_version;
	uint8_t WRAM_data;
	uint32_t WRAM_addr;

	uint8_t PPU1_bus;
	uint8_t PPU2_bus;

	int x, y;
	int queued_cycles;
};

void init_s_ppu(struct S_PPU *s_ppu);
void ppu_dot(struct data_bus *data_bus);

void latch_HVCT(struct data_bus *data_bus);
void clear_HVCT(struct data_bus *data_bus);

#endif // PPU_H

