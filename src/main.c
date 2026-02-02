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

	struct Ricoh_5A22 cpu;
	struct Memory memory;
	struct S_PPU s_ppu;

	data_bus.A_Bus.cpu = &cpu;
	data_bus.A_Bus.memory = &memory;
	data_bus.B_bus.ppu = &s_ppu;

	reset_ricoh_5a22(&data_bus);
	init_memory(&memory, LoROM_MARKER);
	init_s_ppu(&s_ppu);

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
		Interrupted,
	} loop_state = Empty;

	while(!cpu.LPM)
	{
		if(cpu.queued_cyles == 0)
		{
			if(loop_state == Executing)
			{
				if(cpu.IRQ_line == 0)
				{
					loop_state = Interrupted;
				}

				if(cpu.NMI_line == 0)
				{
					loop_state = Interrupted;
				}
			}

			if(loop_state == Empty)
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

		populate_MDMA(&data_bus);
		populate_HDMA(&data_bus);

		if(s_ppu.ppu->queued_cycles == 0)
		{
			ppu_dot(&data_bus);
		}

		if(cpu.RDY)
		{
			cpu.queued_cyles--;
		}
		else 
		{
			if(cpu.NMI_line || cpu.IRQ_line)
			{
				cpu.RDY = 1;
			}
		}
	}

	return 0;
}
