cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
# Initialize the SDK
pico_sdk_init()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
add_executable(main
        Main.cpp
        TCPComm.cpp
        rc-switch-pico/radio-switch.cc
        )


add_compile_options(-Wall
-Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
-Wno-unused-function # we have some for the docs that aren't called
-Wno-maybe-uninitialized
)
target_compile_definitions(main PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    TCP_SERVER_IP=\"${TCP_SERVER_IP}\"
    TCP_PORT=\"${TCP_PORT}\"
)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(main pico_stdlib pico_cyw43_arch_lwip_threadsafe_background pico_multicore)

# enable usb output, disable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)



