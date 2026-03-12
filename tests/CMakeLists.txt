include(FetchContent)
find_package(Threads REQUIRED)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(clearspace_core_tests
  smoke_tests.cpp
  server_core_basic_tests.cpp
  room_stroke_tests.cpp
  room_protocol_tests.cpp
  room_order_tests.cpp
  room_snapshot_tests.cpp
  cursor_rate_tests.cpp
  room_leave_tests.cpp
  concurrency_tests.cpp
  sqlite_snapshot_store_tests.cpp
  snapshot_writer_tests.cpp
)

target_include_directories(clearspace_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(clearspace_core_tests PRIVATE cxx_std_20)

target_link_libraries(clearspace_core_tests
  PRIVATE
    clearspace_core
    clearspace_persistence
    GTest::gtest_main
    Threads::Threads
)

clearspace_enable_warnings(clearspace_core_tests)
clearspace_enable_sanitizer(clearspace_core_tests)

include(GoogleTest)
gtest_discover_tests(clearspace_core_tests)
