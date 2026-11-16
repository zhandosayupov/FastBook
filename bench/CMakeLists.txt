include(FetchContent)
FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

add_executable(fastbook_bench smoke_bench.cpp)
target_include_directories(fastbook_bench PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(fastbook_bench PRIVATE fastbook_warnings benchmark::benchmark_main)
