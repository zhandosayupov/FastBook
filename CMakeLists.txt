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

add_executable(fastbook src/main.cpp)
target_include_directories(fastbook PRIVATE include)
target_link_libraries(fastbook PRIVATE fastbook_warnings)

if(FASTBOOK_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(FASTBOOK_BUILD_BENCH)
  add_subdirectory(bench)
endif()
