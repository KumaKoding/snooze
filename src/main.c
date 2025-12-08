#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "SDL3/SDL.h"

#include "ricoh5A22.h"
#include "memory.h"
#include "DMA.h"
#include "cartridge.h"

int main(int argc, char *argv[])
{
	struct Memory memory;
	init_memory(&memory, LoROM_MARKER);
	load_ROM(argv[1], &memory);

	struct DMA dma;
	init_DMA(&memory);

	struct Ricoh_5A22 cpu;
	reset_ricoh_5a22(&cpu, &memory);

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


	while(1)
	{
		if(cpu.LPM)
		{
			reset_ricoh_5a22(&cpu, &memory);
		}

		while(!cpu.RDY)
		{
			if(DB_read(&memory, 0x4210) == 0x42)
			{
				cpu.RDY = 1;
			}
		}

		uint8_t instruction = fetch(&cpu, &memory);

		populate_MDMA(&dma, &memory);
		populate_HDMA(&dma, &memory);
		
		execute(&cpu, &memory, instruction);

		print_cpu(&cpu);
	}

	return 0;
}
