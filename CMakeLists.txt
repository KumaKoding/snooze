cmake_minimum_required(VERSION 3.10)
project(snooze 
		VERSION 0.0.1
		LANGUAGES C)

add_compile_options(-fsanitize=undefined -Wall -Werror -Iinclude)
add_link_options(-fsanitize=undefined)

find_package(SDL3 REQUIRED)

set(SOURCES src/main.c src/ricoh5A22.c src/memory.c src/cpu_io.c src/ppu_registers.c src/cpu_registers.c src/wram_registers.c src/DMA.c src/cartridge.c src/ppu.c)
set(HEADERS include/ricoh5A22.h include/memory.h include/DMA.h include/cartridge.h include/registers.h include/PPU.h)

include_directories(include)

add_executable(snooze ${SOURCES} ${HEADERS})
target_include_directories(snooze PRIVATE ${HEADERS})
target_link_libraries(snooze PRIVATE SDL3::SDL3)
