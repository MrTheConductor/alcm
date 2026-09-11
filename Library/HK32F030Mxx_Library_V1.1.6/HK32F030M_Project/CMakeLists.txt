cmake_minimum_required(VERSION 3.14)
project(ALCM LANGUAGES C)

include(FetchContent)

FetchContent_Declare(
    doxygen
    URL https://github.com/doxygen/doxygen/releases/download/Release_1_12_0/doxygen-1.12.0.windows.x64.bin.zip
)
FetchContent_MakeAvailable(doxygen)
set(DOXYGEN_EXECUTABLE ${doxygen_SOURCE_DIR}/doxygen.exe)

# Fetch OpenCppCoverage (used by the test_coverage target below) if it isn't
# already installed. GitHub only publishes an NSIS installer for it (no
# portable zip), so FetchContent's archive-unpacking can't be used here -
# download it directly and run it silently instead.
#
# Note: the installer ignores /D and always installs to Program Files
# regardless of the directory requested, and - at least in the environment
# this was verified in - does so without a UAC prompt even from a
# non-elevated shell. That may not hold on every machine, so this is
# best-effort: if the silent install doesn't go through (e.g. UAC declined),
# the test_coverage target is simply skipped rather than failing the build.
set(OPENCPPCOVERAGE_VERSION "0.9.9.0")
find_program(OPENCPPCOVERAGE_EXECUTABLE OpenCppCoverage.exe
    PATHS "C:/Program Files/OpenCppCoverage"
)

if(NOT OPENCPPCOVERAGE_EXECUTABLE)
    set(OPENCPPCOVERAGE_INSTALLER
        "${CMAKE_BINARY_DIR}/OpenCppCoverageSetup-x64-${OPENCPPCOVERAGE_VERSION}.exe")
    message(STATUS "OpenCppCoverage not found, downloading v${OPENCPPCOVERAGE_VERSION}...")
    file(DOWNLOAD
        "https://github.com/OpenCppCoverage/OpenCppCoverage/releases/download/release-${OPENCPPCOVERAGE_VERSION}/OpenCppCoverageSetup-x64-${OPENCPPCOVERAGE_VERSION}.exe"
        "${OPENCPPCOVERAGE_INSTALLER}"
        STATUS OPENCPPCOVERAGE_DOWNLOAD_STATUS
    )
    list(GET OPENCPPCOVERAGE_DOWNLOAD_STATUS 0 OPENCPPCOVERAGE_DOWNLOAD_CODE)

    if(OPENCPPCOVERAGE_DOWNLOAD_CODE EQUAL 0)
        message(STATUS "Installing OpenCppCoverage silently...")
        execute_process(COMMAND "${OPENCPPCOVERAGE_INSTALLER}" /S)
        find_program(OPENCPPCOVERAGE_EXECUTABLE OpenCppCoverage.exe
            PATHS "C:/Program Files/OpenCppCoverage"
        )
    else()
        message(WARNING "Failed to download OpenCppCoverage - the 'test_coverage' target will not be available")
    endif()

    if(NOT OPENCPPCOVERAGE_EXECUTABLE)
        message(WARNING "OpenCppCoverage silent install did not complete (may need admin/UAC approval) - the 'test_coverage' target will not be available")
    endif()
endif()

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
add_library(alcm STATIC
    src/animations.c
    src/battery_lut.c
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

add_library(timer STATIC
    src/timer.c
)

add_library(headlights STATIC
    src/headlights.c
    src/function_generator.c
    src/hysteresis.c
)

add_library(event_queue STATIC
    src/event_queue.c
)

add_library(board_mode STATIC
    src/board_mode.c
    src/hysteresis.c
)

add_library(status_leds STATIC
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
if(OPENCPPCOVERAGE_EXECUTABLE)
    add_custom_target(test_coverage
        COMMAND "${OPENCPPCOVERAGE_EXECUTABLE}" --export_type html:coverage\\headlights --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_headlights.exe
        COMMAND "${OPENCPPCOVERAGE_EXECUTABLE}" --export_type html:coverage\\status_leds --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_status_leds.exe
        COMMAND "${OPENCPPCOVERAGE_EXECUTABLE}" --export_type html:coverage\\board_mode --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_board_mode.exe
        COMMAND "${OPENCPPCOVERAGE_EXECUTABLE}" --export_type html:coverage\\event_queue --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_event_queue.exe
        COMMAND "${OPENCPPCOVERAGE_EXECUTABLE}" --export_type html:coverage\\timer --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_timer.exe
        COMMAND "${OPENCPPCOVERAGE_EXECUTABLE}" --export_type html:coverage\\alcm --sources ${WINDOWS_CMAKE_SOURCE_DIR}  --excluded_sources "D:\\*" --excluded_modules "*.dll" -- ${CMAKE_BINARY_DIR}/tests/Debug/test_alcm.exe
        COMMENT "Generating test coverage"
        VERBATIM
    )
else()
    message(STATUS "OpenCppCoverage not available - 'test_coverage' target not created")
endif()