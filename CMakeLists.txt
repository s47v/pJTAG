# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_BUILD_TYPE "Release")
#set(TINYUSB_DEBUG_LEVEL 0)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pJTAG C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(pJTAG
		pJTAG.c
		pio_jtag.c
)

target_include_directories(pJTAG PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

pico_generate_pio_header(pJTAG ${CMAKE_CURRENT_LIST_DIR}/jtag.pio)

pico_set_program_name(pJTAG "pJTAG")
pico_set_program_version(pJTAG "0.1")

pico_enable_stdio_uart(pJTAG 0)
pico_enable_stdio_usb(pJTAG 1)

# Add the standard library to the build
target_link_libraries(pJTAG PRIVATE pico_stdlib)

# Add any user requested libraries
target_link_libraries(pJTAG PRIVATE
    hardware_pio
	hardware_dma
	pico_unique_id
)

pico_add_extra_outputs(pJTAG)

