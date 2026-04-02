add_library(clearspace_transport
  src/json_protocol.cpp
  src/mappers.cpp
)

target_include_directories(clearspace_transport
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(clearspace_transport PUBLIC cxx_std_20)

target_link_libraries(clearspace_transport
  PUBLIC
    clearspace_core
    nlohmann_json::nlohmann_json
)

target_compile_definitions(clearspace_transport
  PUBLIC
    BOOST_ASIO_DISABLE_LIB
    BOOST_ERROR_CODE_HEADER_ONLY
)

clearspace_enable_warnings(clearspace_transport)
clearspace_enable_sanitizer(clearspace_transport)
