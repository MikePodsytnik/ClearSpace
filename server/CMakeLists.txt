find_package(Threads REQUIRED)

add_library(clearspace_server
  src/session_registry.cpp
  src/client_session.cpp
  src/tcp_server.cpp
)

target_include_directories(clearspace_server
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(clearspace_server PUBLIC cxx_std_20)

target_link_libraries(clearspace_server
  PUBLIC
    clearspace_core
    clearspace_transport
    Boost::headers
    Threads::Threads
)

if (WIN32)
  target_link_libraries(clearspace_server PUBLIC ws2_32)
endif()

target_compile_definitions(clearspace_server
  PUBLIC
    BOOST_ASIO_DISABLE_LIB
    BOOST_ERROR_CODE_HEADER_ONLY
)

clearspace_enable_warnings(clearspace_server)
clearspace_enable_sanitizer(clearspace_server)
