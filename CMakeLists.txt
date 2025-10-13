cmake_minimum_required(VERSION 3.10)
project(snooze 
		VERSION 0.0.1
		LANGUAGES C)

add_compile_options(-fsanitize=undefined -Wall -Werror -Iinclude)
add_link_options(-fsanitize=undefined)

set(SOURCES src/main.c src/Ricoh5A22.c src/memory.c)
set(HEADERS include/Ricoh5A22.h include/memory.h)

include_directories(include)

add_executable(sneeze ${SOURCES} ${HEADERS})
target_include_directories(sneeze PRIVATE ${HEADERS})
