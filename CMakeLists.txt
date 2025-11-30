cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2 CACHE STRING "Board type")
set(PICO_PLATFORM rp2350)

include(pico_sdk_import.cmake)

project(rp_multi_uart_bridge C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(rp_multi_uart_bridge
    src/main.c
    src/usb_descriptors.c
)

pico_generate_pio_header(rp_multi_uart_bridge ${CMAKE_CURRENT_LIST_DIR}/pio/uart_tx.pio)
pico_generate_pio_header(rp_multi_uart_bridge ${CMAKE_CURRENT_LIST_DIR}/pio/uart_rx.pio)

target_include_directories(rp_multi_uart_bridge PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(rp_multi_uart_bridge
    pico_stdlib
    hardware_uart
    hardware_pio
    hardware_clocks
    tinyusb_device
    tinyusb_board
)

# stdio по USB отключаем (иначе лишний CDC)
pico_enable_stdio_usb(rp_multi_uart_bridge 0)
# можно оставить UART stdio для отладки, если хочешь
pico_enable_stdio_uart(rp_multi_uart_bridge 1)

pico_add_extra_outputs(rp_multi_uart_bridge)
