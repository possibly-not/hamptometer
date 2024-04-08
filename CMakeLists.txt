cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(Picow_Hamster C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)
set(PICO_DEBUG_MALLOC 1)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        # -ggdb                # for debugging information
        # -O3
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

set(WIFI_SSID $ENV{WIFI_SSID})
message("Using WIFI_SSID from environment ('${WIFI_SSID}')")
set(WIFI_PASSWORD $ENV{WIFI_PASSWORD} )

set(PICOWOTA_WIFI_SSID WIFI_SSID)
set(PICOWOTA_WIFI_PASS PICOWOTA_WIFI_PASS)

set(PICOWOTA_WIFI_AP 0)

# Add header file directory
include_directories(network)
include_directories(utils)

# Add compilation subdirectory 
add_subdirectory(network)
add_subdirectory(utils)
add_subdirectory(picowota)

add_executable(main
    main.c
)

target_compile_definitions(main PUBLIC
    WIFI_SSID=\"${WIFI_SSID}\"
    PICOWOTA_WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    PICOWOTA_WIFI_PASS=\"${WIFI_PASSWORD}\"
)

target_link_libraries(main PUBLIC
    pico_stdlib
    pico_lwip_http
    hardware_rtc
    hardware_adc
    Network
    Utils
)

target_include_directories(main PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)

picowota_build_combined(main)

