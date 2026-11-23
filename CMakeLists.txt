cmake_minimum_required(VERSION 3.20)
project(FastBook VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

option(FASTBOOK_BUILD_TESTS "Build unit tests" ON)
option(FASTBOOK_BUILD_BENCH "Build benchmarks" ON)

add_library(fastbook_warnings INTERFACE)
target_compile_options(fastbook_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)

# Header-only libraries: no .cpp files, so INTERFACE targets.
add_library(fastbook_utils INTERFACE)
target_include_directories(fastbook_utils INTERFACE include)

# ITCH 5.0 messages, decoder and parser; uses fastbook_utils.
add_library(fastbook_itch INTERFACE)
target_sources(fastbook_itch PRIVATE
  include/fastbook/itch/decode.h
  include/fastbook/itch/parser.h
)
target_link_libraries(fastbook_itch INTERFACE fastbook_utils)

add_library(fastbook_core src/order_book.cpp)
target_include_directories(fastbook_core PUBLIC include)
target_link_libraries(fastbook_core PRIVATE fastbook_warnings)

# The executable must exist before target_link_libraries refers to it.
add_executable(fastbook src/main.cpp)
target_link_libraries(fastbook PRIVATE fastbook_core fastbook_utils fastbook_warnings)

if(FASTBOOK_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(FASTBOOK_BUILD_BENCH)
  add_subdirectory(bench)
endif()
