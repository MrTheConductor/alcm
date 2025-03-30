# Fetch CMocka library (replace with other frameworks if needed)
include(FetchContent)
FetchContent_Declare(
    cmocka
    GIT_REPOSITORY https://git.cryptomilk.org/projects/cmocka.git
    GIT_TAG cmocka-1.1.5
)
set(CMOCKA_BUILD_EXAMPLES OFF CACHE BOOL "Don't build CMocka examples")
set(CMOCKA_BUILD_TESTS OFF CACHE BOOL "Don't build CMocka tests")
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