cmake_minimum_required(VERSION 3.13)

add_compile_definitions(PICO_DEFAULT_UART_BAUD_RATE=921600)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/tinyusb")

include(pico_sdk_import.cmake)

project(spaceorb)

pico_sdk_init()

add_compile_options(-Wall)

add_executable(spaceorb src/spaceorb.c src/descriptors.c)
target_include_directories(spaceorb PRIVATE src)
target_link_libraries(spaceorb pico_stdlib tinyusb_device tinyusb_board)
target_compile_definitions(spaceorb PUBLIC SPACEMOUSE)
pico_add_extra_outputs(spaceorb)

add_executable(spaceorb_joystick src/spaceorb.c src/descriptors.c)
target_include_directories(spaceorb_joystick PRIVATE src)
target_link_libraries(spaceorb_joystick pico_stdlib tinyusb_device tinyusb_board)
target_compile_definitions(spaceorb_joystick PUBLIC JOYSTICK)
pico_add_extra_outputs(spaceorb_joystick)
