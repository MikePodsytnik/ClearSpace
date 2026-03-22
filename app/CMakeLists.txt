add_executable(clearspace_server_app
  src/main.cpp
)

target_compile_features(clearspace_server_app PRIVATE cxx_std_20)

target_link_libraries(clearspace_server_app
  PRIVATE
    clearspace_server
    clearspace_persistence
)

target_compile_definitions(clearspace_server_app
  PRIVATE
    BOOST_ASIO_DISABLE_LIB
    BOOST_ERROR_CODE_HEADER_ONLY
)

clearspace_enable_warnings(clearspace_server_app)
clearspace_enable_sanitizer(clearspace_server_app)
