#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "PPU.h"
#include "SDL3/SDL.h"

#include "ricoh5A22.h"
#include "memory.h"
#include "DMA.h"
#include "cartridge.h"

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
	// init_DMA(&data_bus);


	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *Window = SDL_CreateWindow("Snooze", 640, 480, SDL_WINDOW_OPENGL);

	if(Window == NULL)
	{
		printf(":(\n");
	}

	SDL_Event event;
	bool quit = false;

	while(!quit)
	{
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_EVENT_QUIT)
			{
				quit = true;
			}
		}
	}


	SDL_DestroyWindow(Window);
	SDL_Quit();

	uint8_t instruction = 0x00;
	enum 
	{
		Empty,
		Fetched,
		Executing,
	} loop_state = Empty;


	while(!cpu.LPM)
	{
		if(cpu.queued_cyles == 0)
		{
			if(loop_state == Empty || loop_state == Executing)
			{
				instruction = fetch(&data_bus);
				loop_state = Fetched;
			}

			if(loop_state == Fetched)
			{
				execute(&data_bus, instruction);
				loop_state = Executing;
			}
		}

		if(s_ppu.ppu->queued_cycles == 0)
		{
			ppu_dot(&data_bus);
		}

		if(loop_state != Executing)
		{
			if(cpu.IRQ_line == 0)
			{
				hw_irq(&data_bus);
			}

			if(cpu.NMI_line == 0)
			{
				hw_nmi(&data_bus);
			}
		}

		data_bus.B_bus.ppu->ppu->range_over = 0;
		data_bus.B_bus.ppu->ppu->time_over = 0;

		if(cpu.RDY)
		{
			cpu.queued_cyles--;
		}
	}

	return 0;
}
