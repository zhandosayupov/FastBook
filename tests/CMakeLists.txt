include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(fastbook_tests
  smoke_test.cpp
  order_book_test.cpp
  invariants_test.cpp
)
target_link_libraries(fastbook_tests PRIVATE fastbook_core fastbook_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(fastbook_tests)
