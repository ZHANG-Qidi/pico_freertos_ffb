# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

if(WIN32)
    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../FreeRTOS/FreeRTOS-KernelV11.3.0" CACHE PATH "Path to FreeRTOS Kernel")
else()
    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../FreeRTOS/FreeRTOS-KernelV11.3.1" CACHE PATH "Path to FreeRTOS Kernel")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_freertos_ffb C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include(FreeRTOS_Kernel_import.cmake)

add_subdirectory(ArduinoCore-avr/ArduinoCore-avr-1.8.7)
add_subdirectory(Arduino-FOC/Arduino-FOC-2.2)
add_subdirectory(HID-FFB/HID-FFB-0.1)

add_executable(pico_freertos_ffb
    main.cpp
)

# Reserve the last two 4 KiB sectors of the Pico 2's 4 MiB flash for parameters.
# Override only the linker region; PICO_FLASH_SIZE_BYTES retains physical capacity.
set(FFB_PARAMETER_FLASH_OFFSET 0x3FE000)
set(FFB_FLASH_LINK_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffb_flash")
file(MAKE_DIRECTORY "${FFB_FLASH_LINK_DIR}")
file(WRITE "${FFB_FLASH_LINK_DIR}/pico_flash_region.ld"
    "FLASH(rx) : ORIGIN = 0x10000000, LENGTH = ${FFB_PARAMETER_FLASH_OFFSET}\n")
pico_add_linker_script_override_path(pico_freertos_ffb "${FFB_FLASH_LINK_DIR}"
    FILES pico_flash_region.ld)
target_compile_definitions(hid_ffb PRIVATE FFB_PARAMETER_FLASH_OFFSET=${FFB_PARAMETER_FLASH_OFFSET})

# The current single-core FreeRTOS setup runs on core 0; core 1 is unused.
target_compile_definitions(pico_flash INTERFACE PICO_FLASH_ASSUME_CORE1_SAFE=1)

target_include_directories(pico_freertos_ffb PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(pico_freertos_ffb PRIVATE
        pico_cyw43_arch_none
    )
endif()

# Add any user requested libraries
target_link_libraries(pico_freertos_ffb PRIVATE
    pico_stdlib
    hardware_uart
    hardware_spi
    hardware_i2c
    hardware_irq
    hardware_pwm
    hardware_timer
    hardware_clocks
)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(pico_freertos_ffb PRIVATE
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
)

target_link_libraries(pico_freertos_ffb PRIVATE
    arduino_core
    arduino_foc
    hid_ffb
)

# Example running FreeRTOS on 1 core
# Linking to FreeRTOS-Kernel-Heap4 means we use a dynamic heap for allocations
target_link_libraries(pico_freertos_ffb PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
)
# Set the nunber of cores to 1.
# This defaults to 2 in FreeRTOSConfig_examples_common.h if not defined in here
target_compile_definitions(pico_freertos_ffb PRIVATE
    configNUMBER_OF_CORES=1
)

pico_add_extra_outputs(pico_freertos_ffb)

pico_set_program_name(pico_freertos_ffb "pico_freertos_ffb")
pico_set_program_version(pico_freertos_ffb "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico_freertos_ffb 1)
pico_enable_stdio_usb(pico_freertos_ffb 0)

add_custom_command(TARGET pico_freertos_ffb POST_BUILD
    COMMAND arm-none-eabi-size pico_freertos_ffb.elf
)
