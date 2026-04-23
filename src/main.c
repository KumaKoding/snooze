#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <time.h>

#include "utility.h"
#include "memory.h"
#include "ricoh5A22.h"
#include "PPU.h"
#include "DMA.h"
#include "cartridge.h"

#define SDL_FLAGS SDL_INIT_VIDEO

#define WINDOW_TITLE "snooze"
#define WINDOW_WIDTH DOTS
#define WINDOW_HEIGHT LINES

struct Screen 
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Event event;
	int running;
};

int screen_init_SDL(struct Screen *screen)
{
	if(!SDL_Init(SDL_FLAGS))
	{
		fprintf(stderr, "ERROR initializing SDL3: %s\n", SDL_GetError());

		return 0;
	}

	screen->window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_FLAGS);

	if(!screen->window)
	{
		fprintf(stderr, "ERROR creating SDL3 window: %s\n", SDL_GetError());
		
		return 0;
	}

	screen->renderer = SDL_CreateRenderer(screen->window, NULL);

	if(!screen->renderer)
	{
		fprintf(stderr, "ERROR creating SDL3 renderer: %s\n", SDL_GetError());

		return 0;
	}

	return 1;
}

void free_screen(struct Screen *screen)
{
	if(screen->window)
	{
		SDL_DestroyWindow(screen->window);

		screen->window = NULL;
	}

	if(screen->renderer)
	{
		SDL_DestroyRenderer(screen->renderer);

		screen->renderer = NULL;
	}

	SDL_Quit();
}

void run_snooze(struct Screen *screen)
{
	SDL_SetRenderDrawColor(screen->renderer, 0, 0, 0, 255);

	while(screen->running)
	{
		while(SDL_PollEvent(&screen->event))
		{
			switch (screen->event.type) 
			{
				case SDL_EVENT_QUIT:
					screen->running = 0;

					break;
				default:
					break;
			}
		}

		SDL_RenderClear(screen->renderer);

		SDL_RenderPresent(screen->renderer);

		SDL_Delay(16);
	}
}

int init_snooze(struct Screen *screen)
{
	screen->running = 1;

	if(!screen_init_SDL(screen))
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	struct data_bus data_bus;

	struct Memory memory;
	struct Ricoh_5A22 cpu;
	struct S_PPU s_ppu;
	struct DMA dma;

	data_bus.A_Bus.memory = &memory;
	data_bus.A_Bus.cpu = &cpu;
	data_bus.B_bus.ppu = &s_ppu;
	data_bus.B_bus.dma = &dma;

	init_memory(&memory, LoROM_MARKER);
	if(argc > 1)
	{
		load_ROM(argv[1], &data_bus);
	}
	reset_ricoh_5a22(&data_bus);
	init_s_ppu(&s_ppu);
	init_DMA(&data_bus);

	//
	// struct Screen screen = { 0 };
	//
	// int exit_status = init_snooze(&screen);
	//
	// run_snooze(&screen);

	int DMA_alignment_counter = 0;
	uint8_t instruction = 0x00;
	enum 
	{
		Empty,
		Fetched,
	} loop_state = Empty;

	while(!cpu.LPM)
	{
		cpu.NMI_line = !(cpu.internal_registers.NMIEN & cpu.internal_registers.NMI_flag);
		cpu.IRQ_line = !((cpu.internal_registers.IRQEN != DISABLE) & cpu.internal_registers.IRQ_flag) || check_bit8(cpu.cpu_status, CPU_STATUS_I);

		if(cpu.queued_cyles == 0 && !dma.dma_active)
		{
			if(loop_state == Empty)
			{
				instruction = fetch(&data_bus);
				loop_state = Fetched;
			}
			else if(loop_state == Fetched)
			{
				execute(&data_bus, instruction);
				loop_state = Empty;
			}

			if(cpu.REFRESH)
			{
				cpu.queued_cyles += 40;

				cpu.REFRESH = 0;
			}
		}

		if(s_ppu.ppu->queued_cycles == 0)
		{
			ppu_dot(&data_bus);
		}

		if(loop_state == Empty)
		{
			if(cpu.NMI_line == 0)
			{
				hw_nmi(&data_bus);
			}
			else if(cpu.IRQ_line == 0)
			{
				hw_irq(&data_bus);
			}
		}

		DMA_alignment_counter++;

		if(DMA_alignment_counter == 8)
		{
			DMA_alignment_counter = 0;
		}

		if(loop_state == Fetched)
		{
			if(dma.queued_cycles == 0)
			{
				DMA_transfers(&data_bus, DMA_alignment_counter);
				cpu.queued_cyles += dma.queued_cycles;
			}
		}

		if(cpu.RDY)
		{
			cpu.queued_cyles--;
		}

		if(dma.dma_active)
		{
			dma.queued_cycles--;
		}

		s_ppu.ppu->queued_cycles--;
	}
	
	
	// free_screen(&screen);

	// return exit_status;
	

	return 0;
}
