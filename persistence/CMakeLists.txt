add_library(clearspace_persistence
  src/sqlite_snapshot_store.cpp
  src/snapshot_writer.cpp
)

target_include_directories(clearspace_persistence
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(clearspace_persistence PUBLIC cxx_std_20)

find_package(SQLite3 REQUIRED)

set(CLEARSPACE_SQLITE_TARGET "")
if (TARGET SQLite3::SQLite3)
  set(CLEARSPACE_SQLITE_TARGET SQLite3::SQLite3)
elseif (TARGET SQLite::SQLite3)
  set(CLEARSPACE_SQLITE_TARGET SQLite::SQLite3)
endif()

if (CLEARSPACE_SQLITE_TARGET STREQUAL "")
  message(FATAL_ERROR "SQLite3 package found but no imported target is available.")
endif()

target_link_libraries(clearspace_persistence
  PUBLIC
    clearspace_core
    ${CLEARSPACE_SQLITE_TARGET}
)

clearspace_enable_warnings(clearspace_persistence)
clearspace_enable_sanitizer(clearspace_persistence)
