cmake_minimum_required(VERSION 3.14)
project(ALCM LANGUAGES C)

include(FetchContent)

FetchContent_Declare(
    doxygen
    URL https://github.com/doxygen/doxygen/releases/download/Release_1_12_0/doxygen-1.12.0.windows.x64.bin.zip
)
FetchContent_MakeAvailable(doxygen)
set(DOXYGEN_EXECUTABLE ${doxygen_SOURCE_DIR}/doxygen.exe)

# Add coverage flags for GCC or Clang
if(CMAKE_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# Enable clang-tidy
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

# Add the main source code
add_library(alcm
    src/animations.c
    src/button_driver.c
    src/button_events.c
    src/buzzer.c
    src/command_processor.c
    src/crc16_ccitt.c
    src/footpads.c
    src/function_generator.c
    src/hysteresis.c
    src/power.c
    src/ring_buffer.c
    src/vesc_serial.c
)

add_library(timer
    src/timer.c
)

add_library(headlights
    src/headlights.c
    src/function_generator.c
    src/hysteresis.c
)

add_library(event_queue
    src/event_queue.c
)

add_library(board_mode
    src/board_mode.c
    src/hysteresis.c
)

add_library(status_leds
    src/status_leds.c
)

target_include_directories(alcm PUBLIC inc tests/mocks)
target_include_directories(timer PUBLIC inc tests/mocks)
target_include_directories(event_queue PUBLIC inc tests/mocks)
target_include_directories(board_mode PUBLIC inc tests/mocks)
target_include_directories(status_leds PUBLIC inc tests/mocks)
target_include_directories(headlights PUBLIC inc tests/mocks)

# Enable testing
enable_testing()

# Add the tests
add_subdirectory(tests)

# Doxygen dirs
set(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs)
set(DOXYGEN_INPUT_DIR ${CMAKE_SOURCE_DIR}/src)

# Configure the Doxyfile
configure_file(
    ${CMAKE_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_BINARY_DIR}/Doxyfile @ONLY
)

# Add the documentation
add_custom_target(alcm_doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

file(TO_NATIVE_PATH "$(CMAKE_SOURCE_DIR)/src" WINDOWS_CMAKE_SOURCE_DIR)
message(STATUS "${WINDOWS_CMAKE_SOURCE_DIR}")

# Add test coverage
add_custom_target(test_coverage
    COMMAND C:/Program\ Files/OpenCppCoverage/OpenCppCoverage.exe --export_type html:coverage\\headlights --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_headlights.exe
    COMMAND C:/Program\ Files/OpenCppCoverage/OpenCppCoverage.exe --export_type html:coverage\\status_leds --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_status_leds.exe
    COMMAND C:/Program\ Files/OpenCppCoverage/OpenCppCoverage.exe --export_type html:coverage\\board_mode --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_board_mode.exe
    COMMAND C:/Program\ Files/OpenCppCoverage/OpenCppCoverage.exe --export_type html:coverage\\event_queue --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_event_queue.exe
    COMMAND C:/Program\ Files/OpenCppCoverage/OpenCppCoverage.exe --export_type html:coverage\\timer --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_timer.exe
    COMMAND C:/Program\ Files/OpenCppCoverage/OpenCppCoverage.exe --export_type html:coverage\\alcm --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_alcm.exe
    COMMENT "Generating test coverage"
    VERBATIM
)