find_package(Threads REQUIRED)
find_package(GTest QUIET)

if (NOT GTest_FOUND)
    if (EXISTS "${PROJECT_SOURCE_DIR}/cmake-build-debug/_deps/googletest-src/CMakeLists.txt")
        add_subdirectory("${PROJECT_SOURCE_DIR}/cmake-build-debug/_deps/googletest-src" "${CMAKE_BINARY_DIR}/googletest-local")
    else()
        include(FetchContent)
        FetchContent_Declare(
                googletest
                URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

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
        transport_protocol_tests.cpp
        transport_mapper_tests.cpp
        tcp_server_integration_tests.cpp
)

target_include_directories(clearspace_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(clearspace_core_tests PRIVATE cxx_std_20)

target_link_libraries(clearspace_core_tests
        PRIVATE
        clearspace_core
        clearspace_persistence
        clearspace_transport
        clearspace_server
        GTest::gtest_main
        Threads::Threads
)

target_compile_definitions(clearspace_core_tests
        PRIVATE
        BOOST_ASIO_DISABLE_LIB
        BOOST_ERROR_CODE_HEADER_ONLY
)

clearspace_enable_warnings(clearspace_core_tests)
clearspace_enable_sanitizer(clearspace_core_tests)

include(GoogleTest)
gtest_discover_tests(clearspace_core_tests)
