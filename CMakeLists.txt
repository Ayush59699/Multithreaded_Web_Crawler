cmake_minimum_required(VERSION 3.10)
project(WebCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Option to enable/disable debug dumps at build time
option(ENABLE_DUMP "Enable debug dump utilities (controls DEBUG_DUMP macro)" ON)
if(ENABLE_DUMP)
    message(STATUS "DEBUG_DUMP enabled")
    add_compile_definitions(DEBUG_DUMP=1)
else()
    message(STATUS "DEBUG_DUMP disabled")
    add_compile_definitions(DEBUG_DUMP=0)
endif()

# Source files - using absolute paths for safety
set(CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/downloader.cpp"
    "${CMAKE_SOURCE_DIR}/src/parser.cpp"
    "${CMAKE_SOURCE_DIR}/src/url_frontier.cpp"
    "${CMAKE_SOURCE_DIR}/src/storage_manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/thread_manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils.cpp"
    "${CMAKE_SOURCE_DIR}/src/DumpUtils.cpp"
    "${CMAKE_SOURCE_DIR}/src/crawler_config.cpp"
    "${CMAKE_SOURCE_DIR}/src/metrics_collector.cpp"
)

# Core object library to share between crawler and benchmark_runner
add_library(crawler_core OBJECT ${CORE_SOURCES})

# Create executable crawler
add_executable(crawler $<TARGET_OBJECTS:crawler_core> "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Create executable benchmark_runner
add_executable(benchmark_runner $<TARGET_OBJECTS:crawler_core> "${CMAKE_SOURCE_DIR}/src/benchmark_runner.cpp")

# Link libraries
target_link_libraries(crawler PRIVATE 
    ${CURL_LIBRARIES}
    Threads::Threads
)
target_link_libraries(benchmark_runner PRIVATE 
    ${CURL_LIBRARIES}
    Threads::Threads
)

# Windows psapi dependency for process memory/cpu times
if(WIN32)
    target_link_libraries(crawler PRIVATE psapi)
    target_link_libraries(benchmark_runner PRIVATE psapi)
    target_compile_options(crawler PRIVATE /utf-8)
    target_compile_options(benchmark_runner PRIVATE /utf-8)
    target_compile_options(crawler_core PRIVATE /utf-8)
endif()

# Include curl headers
target_include_directories(crawler PRIVATE ${CURL_INCLUDE_DIRS})
target_include_directories(benchmark_runner PRIVATE ${CURL_INCLUDE_DIRS})
target_include_directories(crawler_core PRIVATE ${CURL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

# Build-time toggle for debug dump outputs (defines DEBUG_DUMP)
option(ENABLE_DUMP "Enable debug dump outputs (defines DEBUG_DUMP)" ON)

if(ENABLE_DUMP)
    target_compile_definitions(crawler PRIVATE DEBUG_DUMP=1)
    target_compile_definitions(benchmark_runner PRIVATE DEBUG_DUMP=1)
    target_compile_definitions(crawler_core PRIVATE DEBUG_DUMP=1)
else()
    target_compile_definitions(crawler PRIVATE DEBUG_DUMP=0)
    target_compile_definitions(benchmark_runner PRIVATE DEBUG_DUMP=0)
    target_compile_definitions(crawler_core PRIVATE DEBUG_DUMP=0)
endif()

# Compiler flags
if(UNIX)
    target_compile_options(crawler PRIVATE -Wall -Wextra -O2)
    target_compile_options(benchmark_runner PRIVATE -Wall -Wextra -O2)
    target_compile_options(crawler_core PRIVATE -Wall -Wextra -O2)
endif()

# Custom clean target
add_custom_target(clean-all
    COMMAND ${CMAKE_MAKE_PROGRAM} clean
    COMMAND rm -f ${CMAKE_BINARY_DIR}/crawler
    COMMAND rm -f ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND rm -f ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND rm -f ${CMAKE_BINARY_DIR}/Makefile
    COMMAND echo "Clean complete - all build artifacts removed"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print configuration
message(STATUS "=== Build Configuration ===")
message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build dir: ${CMAKE_BINARY_DIR}")
message(STATUS "Include dir: ${CMAKE_SOURCE_DIR}/include")
message(STATUS "CURL version: ${CURL_VERSION_STRING}")
message(STATUS "CURL libraries: ${CURL_LIBRARIES}")
message(STATUS "Source files:")
foreach(SRC ${SOURCES})
    message(STATUS "  - ${SRC}")
endforeach()
message(STATUS "=== Available Targets ===")
message(STATUS "  make             - Build crawler")
message(STATUS "  make clean       - Remove .o object files")
message(STATUS "  make clean-all   - Remove everything (executable + CMake files)")
message(STATUS "=========================")