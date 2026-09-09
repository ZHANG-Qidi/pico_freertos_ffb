file(GLOB_RECURSE HID_FFB_SRC
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_library(hid_ffb STATIC
    ${HID_FFB_SRC}
)

target_include_directories(hid_ffb PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(hid_ffb PRIVATE
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
)

target_link_libraries(hid_ffb PUBLIC
    arduino_core
    arduino_foc
)

# Example running FreeRTOS on 1 core
# Linking to FreeRTOS-Kernel-Heap4 means we use a dynamic heap for allocations
target_link_libraries(hid_ffb PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
)
# Set the nunber of cores to 1.
# This defaults to 2 in FreeRTOSConfig_examples_common.h if not defined in here
target_compile_definitions(hid_ffb PRIVATE
    configNUMBER_OF_CORES=1
)
