cmake_minimum_required(VERSION 3.15)
project(ALCM_Library C)

# Build as shared library (DLL on Windows)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(BUILD_SHARED_LIBS ON)

# Fix for PDB locking issues with parallel builds on MSVC
if(MSVC)
    add_compile_options(/FS)
endif()

# Paths to ALCM source
set(ALCM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../Library/HK32F030Mxx_Library_V1.1.6/HK32F030M_Project")
set(ALCM_SRC "${ALCM_ROOT}/src")
set(ALCM_INC "${ALCM_ROOT}/inc")

# Include directories - OUR STUBS FIRST, then ALCM headers
include_directories(
    # Our stub headers (searched first to override hardware headers)
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/cmsis
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/hw_interface
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    
    # ALCM headers
   ${ALCM_INC}
)

# ALCM application logic source files (NOT *_hw.c, NOT main.c, NOT hk32f030m_it.c, NOT tim1.c, NOT eeprom.c, NOT ws2812.s)
set(ALCM_SOURCES
    ${ALCM_SRC}/animations.c
    ${ALCM_SRC}/board_mode.c
    ${ALCM_SRC}/button_driver.c
    ${ALCM_SRC}/button_events.c
    ${ALCM_SRC}/buzzer.c
    ${ALCM_SRC}/command_processor.c
    ${ALCM_SRC}/crc16_ccitt.c
    ${ALCM_SRC}/event_queue.c
    ${ALCM_SRC}/footpads.c
    ${ALCM_SRC}/function_generator.c
    ${ALCM_SRC}/headlights.c
    ${ALCM_SRC}/hysteresis.c
    ${ALCM_SRC}/power.c
    ${ALCM_SRC}/ring_buffer.c
    ${ALCM_SRC}/settings.c
    ${ALCM_SRC}/status_leds.c
    ${ALCM_SRC}/timer.c
    ${ALCM_SRC}/vesc_serial.c
)

# Platform stub sources
set(PLATFORM_SOURCES
    platform/system_hk32f030m.c
    platform/systick_sim.c
    platform/hal/hk32f030m_gpio.c
    platform/hal/hk32f030m_rcc.c
    platform/hal/hk32f030m_tim.c
    platform/hal/hk32f030m_adc.c
    platform/hal/hk32f030m_usart.c
    platform/hal/hk32f030m_flash.c
    platform/hal/hk32f030m_exti.c
)

# Hardware layer implementations (REPLACE the original *_hw.c files)
set(HW_LAYER_SOURCES
    hw_impl/status_leds_hw.c
    hw_impl/headlights_hw.c
    hw_impl/footpads_hw.c
    hw_impl/button_driver_hw.c
    hw_impl/vesc_serial_hw.c
    hw_impl/buzzer_hw.c
    hw_impl/power_hw.c
    hw_impl/interrupts_hw.c
    hw_impl/tim1.c
    hw_impl/eeprom.c
    hw_impl/hk32f030m_it.c
)

# Hardware interface sources
set(HW_INTERFACE_SOURCES
    hw_interface/hw_callbacks.c
    hw_interface/hw_inputs.c
)

# API sources (to be created next)
set(API_SOURCES
    api/alcm_api.c
)

# Create shared library
add_library(ALCM.Library SHARED
    ${ALCM_SOURCES}
    ${PLATFORM_SOURCES}
    ${HW_LAYER_SOURCES}
    ${HW_INTERFACE_SOURCES}
    ${API_SOURCES}
)

# Compiler options
if(MSVC)
    target_compile_options(ALCM.Library PRIVATE /W3)
    # Disable MSVC warnings about standard C functions
    target_compile_definitions(ALCM.Library PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(ALCM.Library PRIVATE -Wall -Wextra)
endif()

# Define required preprocessor symbols
target_compile_definitions(ALCM.Library PRIVATE
    # Enable features
    ENABLE_STATUS_LEDS
    ENABLE_BUZZER
    
    # Use standard types
    USE_STDPERIPH_DRIVER
)

# Set output directory
set_target_properties(ALCM.Library PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Install rules
install(TARGETS ALCM.Library
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
