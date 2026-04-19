include(FetchContent)
include(CMakeDependentOption)

function(clearspace_use_system_or_fetch_package package_name)
  string(TOUPPER "${package_name}" package_upper)
  if (CLEARSPACE_DEPS_MODE STREQUAL "system" OR CLEARSPACE_DEPS_MODE STREQUAL "auto")
    set(system_flag QUIET)
  else()
    set(system_flag)
  endif()

  if (package_name STREQUAL "nlohmann_json")
    find_package(nlohmann_json ${system_flag} CONFIG)
    if (nlohmann_json_FOUND)
      return()
    endif()

    if (CLEARSPACE_DEPS_MODE STREQUAL "system")
      message(FATAL_ERROR "nlohmann_json not found in system packages.")
    endif()

    FetchContent_Declare(
      nlohmann_json
      URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
    return()
  endif()

  if (package_name STREQUAL "Boost")
    find_package(Boost ${system_flag} CONFIG)
    if (Boost_FOUND AND TARGET Boost::headers)
      return()
    endif()

    if (CLEARSPACE_DEPS_MODE STREQUAL "system")
      message(FATAL_ERROR "Boost headers target not found in system packages.")
    endif()

    FetchContent_Declare(
      boost
      URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BOOST_INCLUDE_LIBRARIES asio filesystem system)
    set(BOOST_ENABLE_CMAKE ON)
    FetchContent_MakeAvailable(boost)

    if (NOT TARGET Boost::headers)
      add_library(Boost::headers INTERFACE IMPORTED)
      target_include_directories(Boost::headers INTERFACE "${boost_SOURCE_DIR}")
    endif()
    return()
  endif()

  if (package_name STREQUAL "SQLite3")
    find_package(SQLite3 ${system_flag})
    if (SQLite3_FOUND AND (TARGET SQLite::SQLite3 OR TARGET SQLite3::SQLite3))
      return()
    endif()

    if (CLEARSPACE_DEPS_MODE STREQUAL "system")
      message(FATAL_ERROR "SQLite3 not found in system packages.")
    endif()

    FetchContent_Declare(
      sqlite3_src
      URL https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_GetProperties(sqlite3_src)
    if (NOT sqlite3_src_POPULATED)
      FetchContent_Populate(sqlite3_src)
    endif()

    add_library(clearspace_sqlite3 STATIC
      "${sqlite3_src_SOURCE_DIR}/sqlite3.c"
    )
    target_include_directories(clearspace_sqlite3 PUBLIC "${sqlite3_src_SOURCE_DIR}")
    if (WIN32)
      target_compile_definitions(clearspace_sqlite3 PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    add_library(SQLite::SQLite3 ALIAS clearspace_sqlite3)
    return()
  endif()

  if (package_name STREQUAL "GTest")
    find_package(GTest ${system_flag} CONFIG)
    if (GTest_FOUND)
      return()
    endif()

    if (CLEARSPACE_DEPS_MODE STREQUAL "system")
      message(FATAL_ERROR "GTest not found in system packages.")
    endif()

    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    return()
  endif()

  message(FATAL_ERROR "Unsupported package request: ${package_name}")
endfunction()

function(clearspace_setup_qt)
  if (NOT CLEARSPACE_ENABLE_QT_CLIENT)
    return()
  endif()

  if (CLEARSPACE_QT_MODE STREQUAL "static-source")
    include(ClearSpaceStaticQt)
    clearspace_prepare_static_qt()
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets
      PATHS "${CLEARSPACE_QT_STATIC_INSTALL_DIR}"
      "${CLEARSPACE_QT_STATIC_INSTALL_DIR}/lib/cmake"
      NO_DEFAULT_PATH)
    return()
  endif()

  find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
endfunction()

function(clearspace_setup_common_dependencies)
  clearspace_use_system_or_fetch_package(nlohmann_json)
  clearspace_use_system_or_fetch_package(Boost)
  clearspace_use_system_or_fetch_package(SQLite3)
  clearspace_setup_qt()
endfunction()
