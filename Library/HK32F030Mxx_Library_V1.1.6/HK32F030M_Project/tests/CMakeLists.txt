# Fetch CMocka library (replace with other frameworks if needed)
include(FetchContent)
FetchContent_Declare(
    cmocka
    GIT_REPOSITORY https://gitlab.com/cmocka/cmocka.git
    GIT_TAG cmocka-1.1.7
)
set(WITH_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)  # Disable examples to avoid building them
FetchContent_MakeAvailable(cmocka)


# Add test executable for status LEDs 
add_executable(test_status_leds
    status_leds_main.c
    mocks/mock_event_queue.c
    mocks/mock_footpads.c
    mocks/mock_vesc_serial.c
    mocks/mock_animations.c
    mocks/mock_settings.c
    mocks/mock_board_mode.c
    mocks/mock_timer.c
    mocks/mock_status_leds_hw.c
    )

target_link_libraries(test_status_leds PRIVATE status_leds cmocka)

# Enable testing and add test
add_test(NAME StatusLedsTest COMMAND test_status_leds)

# Add test executable for the actual timer
add_executable(test_timer
    timer_main.c
    mocks/mock_event_queue.c
    )

target_link_libraries(test_timer PRIVATE timer cmocka)

# Enable testing and add test
add_test(NAME TimerTest COMMAND test_timer)

# Add test executable for the headlights 
add_executable(test_headlights
    headlights_main.c
    mocks/mock_event_queue.c
    mocks/mock_timer.c
    mocks/mock_settings.c
    mocks/mock_board_mode.c
    mocks/mock_vesc_serial.c
    mocks/mock_headlights_hw.c
    )

target_link_libraries(test_headlights PRIVATE headlights cmocka)

# Enable testing and add test
add_test(NAME HeadlightTest COMMAND test_headlights)

# Add test executable for the actual event queue
add_executable(test_event_queue
    event_queue_main.c
    mocks/mock_interrupts.c
    )

target_link_libraries(test_event_queue PRIVATE event_queue cmocka)

# Enable testing and add test
add_test(NAME EventQueueTest COMMAND test_event_queue)

# Add test executable for the actual board_mode
add_executable(test_board_mode
    board_mode_main.c
    mocks/mock_event_queue.c
    mocks/mock_timer.c
    mocks/mock_footpads.c
    mocks/mock_vesc_serial.c
    )

target_link_libraries(test_board_mode PRIVATE board_mode cmocka)

# Enable testing and add test
add_test(NAME BoardModeTest COMMAND test_board_mode)

# Add test executable for hysteresis
add_executable(test_hysteresis
    hysteresis_main.c
    )

target_link_libraries(test_hysteresis PRIVATE hysteresis cmocka)

# Enable testing and add test
add_test(NAME HysteresisTest COMMAND test_hysteresis)

# Add test executable for button_events
add_executable(test_button_events
    button_events_main.c
    mocks/mock_event_queue.c
    mocks/mock_timer.c
    )

target_link_libraries(test_button_events PRIVATE button_events cmocka)

# Enable testing and add test
add_test(NAME ButtonEventsTest COMMAND test_button_events)

# Add test executable for button_driver
add_executable(test_button_driver
    button_driver_main.c
    mocks/mock_event_queue.c
    mocks/mock_timer.c
    mocks/mock_button_driver_hw.c
    )

target_link_libraries(test_button_driver PRIVATE button_driver cmocka)

# Enable testing and add test
add_test(NAME ButtonDriverTest COMMAND test_button_driver)

# Add test executable for settings
add_executable(test_settings
    settings_main.c
    mocks/mock_eeprom.c
    mocks/mock_event_queue.c
    )

target_link_libraries(test_settings PRIVATE settings cmocka)

# Enable testing and add test
add_test(NAME SettingsTest COMMAND test_settings)

# Add test executable for command_processor
add_executable(test_command_processor
    command_processor_main.c
    mocks/mock_event_queue.c
    mocks/mock_timer.c
    mocks/mock_board_mode.c
    mocks/mock_settings.c
    mocks/mock_button_driver_hw.c
    )

target_link_libraries(test_command_processor PRIVATE command_processor cmocka)

# Enable testing and add test
add_test(NAME CommandProcessorTest COMMAND test_command_processor)

# Add test executable for mocked components
add_executable(test_alcm
    mocks/mock_board_mode.c
    mocks/mock_eeprom.c
    mocks/mock_buzzer_hw.c
    mocks/mock_event_queue.c
    mocks/mock_footpads_hw.c
    mocks/mock_headlights_hw.c
    mocks/mock_interrupts.c
    mocks/mock_power_hw.c
    mocks/mock_status_leds_hw.c
    mocks/mock_timer.c
    mocks/mock_vesc_serial_hw.c
    mocks/mock_battery_lut_hw.c
    mocks/mock_settings.c
    mocks/mock_status_leds.c
    alcm_main.c
)

target_include_directories(test_alcm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include-internal)
target_link_libraries(test_alcm PRIVATE alcm cmocka)


# Enable testing and add test
add_test(NAME ALCMTest COMMAND test_alcm)


# Copy CMocka DLL
add_custom_command(
    TARGET test_alcm 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cmocka>  # Path to the CMocka DLL
        $<TARGET_FILE_DIR:test_alcm>  # Test executable's folder
)