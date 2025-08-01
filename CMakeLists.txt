

# ====================================================================================
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
set(FREERTOS_KERNEL_PATH "C:/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(Datalogger C CXX ASM)

pico_sdk_init()

include_directories(${CMAKE_SOURCE_DIR}/lib)

add_subdirectory(lib/FatFs_SPI)    

add_executable(Datalogger 
    Datalogger.c 
    lib/ssd1306.c # Biblioteca para o display OLED
    lib/mpu6050.c # Biblioteca para o MPU6050
    lib/hw_config.c

)

pico_set_program_name(Datalogger "Datalogger")
pico_set_program_version(Datalogger "0.1")

pico_enable_stdio_uart(Datalogger 0)
pico_enable_stdio_usb(Datalogger 1)

target_link_libraries(Datalogger
        pico_stdlib
        hardware_i2c
        hardware_gpio
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4
        FatFs_SPI
        hardware_clocks
        hardware_adc
)

target_include_directories(Datalogger PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(Datalogger)

