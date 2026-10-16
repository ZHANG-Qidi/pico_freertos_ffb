file(GLOB_RECURSE ARDUINO_CORE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_library(arduino_core STATIC
    ${ARDUINO_CORE_SRC}
)

target_include_directories(arduino_core PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/cores/arduino
    ${CMAKE_CURRENT_LIST_DIR}/libraries/Wire/src
    ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI/src
    ${CMAKE_CURRENT_LIST_DIR}/libraries/USB/src
    ${CMAKE_CURRENT_LIST_DIR}/libraries/EEPROM/src
    ${CMAKE_SOURCE_DIR}
)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(arduino_core PRIVATE
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
)

target_link_libraries(arduino_core PRIVATE
    hardware_uart
    hardware_spi
    hardware_i2c
    hardware_irq
    hardware_pwm
    hardware_timer
    hardware_clocks
    hardware_pio
    hardware_adc
    hardware_flash
    pico_flash
)

# Example running FreeRTOS on 1 core
# Linking to FreeRTOS-Kernel-Heap4 means we use a dynamic heap for allocations
target_link_libraries(arduino_core PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
)
# Set the nunber of cores to 1.
# This defaults to 2 in FreeRTOSConfig_examples_common.h if not defined in here
target_compile_definitions(arduino_core PRIVATE
    configNUMBER_OF_CORES=1
)

# Generate PIO header
pico_generate_pio_header(arduino_core ${CMAKE_CURRENT_LIST_DIR}/cores/arduino/svpwm.pio)
pico_generate_pio_header(arduino_core ${CMAKE_CURRENT_LIST_DIR}/cores/arduino/pwm.pio)

# Call after creating the executable that uses this compatibility layer.
function(arduino_configure_eeprom TARGET)
    # Reserve the last 4 KiB sector of the Pico 2's 4 MiB flash for EEPROM.
    # Override only the linker region; PICO_FLASH_SIZE_BYTES retains physical capacity.
    set(PICO_EEPROM_FLASH_OFFSET 0x3FF000)
    set(EEPROM_FLASH_LINK_DIR "${CMAKE_CURRENT_BINARY_DIR}/eeprom_flash")
    file(MAKE_DIRECTORY "${EEPROM_FLASH_LINK_DIR}")
    file(WRITE "${EEPROM_FLASH_LINK_DIR}/pico_flash_region.ld"
        "FLASH(rx) : ORIGIN = 0x10000000, LENGTH = ${PICO_EEPROM_FLASH_OFFSET}\n")
    pico_add_linker_script_override_path(${TARGET} "${EEPROM_FLASH_LINK_DIR}"
        FILES pico_flash_region.ld)
    target_compile_definitions(arduino_core PRIVATE PICO_EEPROM_FLASH_OFFSET=${PICO_EEPROM_FLASH_OFFSET})

    # The current single-core FreeRTOS setup runs on core 0; core 1 is unused.
    target_compile_definitions(pico_flash INTERFACE PICO_FLASH_ASSUME_CORE1_SAFE=1)
endfunction()
