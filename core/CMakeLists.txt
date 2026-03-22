add_library(clearspace_core
  src/room.cpp
  src/room_manager.cpp
  src/server_core.cpp
  src/time.cpp
)

target_include_directories(clearspace_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(clearspace_core PUBLIC cxx_std_20)

clearspace_enable_warnings(clearspace_core)
clearspace_enable_sanitizer(clearspace_core)
