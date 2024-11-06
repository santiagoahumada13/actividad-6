cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(oled CXX C ASM)

set(PICO_BOARD pico_w)
set(CMAKE_C_STANDARD11)
set(CMAKE_CXX_STANDARD17)

pico_sdk_init()

add_executable(oled
oled.c
)

target_link_libraries(oled
	pico_stdlib
	hardware_i2c
	)
pico_add_extra_outputs(oled)