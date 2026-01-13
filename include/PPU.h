#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#define DOTS 256
#define LINES 239

#define VRAM_WORD_WIDTH 2

#define M0_BACKGROUNDS 4

#define INIDISP 0x2100
#define OBJSEL 0x2101
#define OAMADDL 0x2102
#define OAMADDH 0x2103
#define OAMDATA 0x2104
#define BGMODE 0x2105
#define MOSAIC 0x2106
#define BG1SC 0x2107
#define BG2SC 0x2108
#define BG3SC 0x2109
#define BG4SC 0x210A
#define BG12NBA 0x210B
#define BG34NBA 0x210C
#define BG1HOFS 0x210D
#define M7HOFS 0x210D
#define BG1VOFS 0x210E
#define M7VOFS 0x210E
#define BG2HOFS 0x210F
#define BG2VOFS 0x2110
#define BG3HOFS 0x2111
#define BG3VOFS 0x2112
#define BG4HOFS 0x2113
#define BG4VOFS 0x2114
#define VMAIN 0x2115
#define VMADDL 0x002116
#define VMADDH 0x002117
#define VMDATAL 0x002118
#define VMDATAH 0x002119
#define M7SEL 0x211A
#define M7A 0x211B
#define M7B 0x211C
#define M7C 0x211D
#define M7D 0x211E
#define M7X 0x211F
#define M7Y 0x2120
#define CGADD 0x002121
#define CGDATA 0x002122
#define W12SEL 0x2123
#define W34SEL 0x2124
#define WOBJSEL 0x2125
#define WH0 0x2126
#define WH1 0x2127
#define WH2 0x2128
#define WH3 0x2129
#define WBGLOG 0x212A
#define WOBJLOG 0x212B
#define TM 0x212C
#define TS 0x212D
#define TMW 0x212E
#define TSW 0x212F
#define CGWSEL 0x2130
#define CGADSUB 0x2131
#define COLDATA 0x2132
#define SETINI 0x2133
#define MPYL 0x2134
#define MPYM 0x2135
#define MPYH 0x2136
#define SLHV 0x2137
#define OAMDATAREAD 0x002138
#define VMDATALREAD 0x002139
#define VMDATAHREAD 0x002139
#define CGDATAREAD 0x00213B
#define OPHCT 0x213B
#define OPVCT 0x213D
#define STAT77 0x213E
#define STAT78 0x213F

#define WRAM_WRITE 0x002180
#define WRAM_READ 0x002180
#define WRAM_ADDR_LO 0x002181
#define WRAM_ADDR_HI 0x002182
#define WRAM_ADDR_BK 0x002183

#define NMI 0x4210

#define VRAM_WORDS 32 * 1024
#define OAM_LTABLE_BYTES 256 * 2
#define OAM_HTABLE_BYTES 16 * 2 
#define CGRAM_WORDS 256 * 2

struct PPU_memory
{
	uint8_t *VRAM;
	uint8_t *OAM_low_table;
	uint8_t *OAM_high_table;
	uint8_t *CGRAM;
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
	OR,
	AND,
	XOR,
	XNOR
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

	uint8_t PPU1_bus;
	uint8_t PPU2_bus;

	int x, y;
};

#endif // PPU_H

