cmake_minimum_required(VERSION 3.20)

if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(clearspace
  VERSION 0.2.0
  DESCRIPTION "ClearSpace core + persistence + network API"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLEARSPACE_ENABLE_TESTS "Build tests" ON)

set(CLEARSPACE_SANITIZER "none" CACHE STRING "Sanitizer: none|asan|tsan|ubsan")
set_property(CACHE CLEARSPACE_SANITIZER PROPERTY STRINGS none asan tsan ubsan)

find_package(Boost CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

function(clearspace_enable_warnings target_name)
  if (MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target_name} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Woverloaded-virtual
      -Wnull-dereference
    )
  endif()
endfunction()

function(clearspace_enable_sanitizer target_name)
  if (CLEARSPACE_SANITIZER STREQUAL "none")
    return()
  endif()

  if (MSVC)
    message(WARNING "Sanitizers via flags are not configured for MSVC in this template.")
    return()
  endif()

  if (CLEARSPACE_SANITIZER STREQUAL "asan")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
  elseif (CLEARSPACE_SANITIZER STREQUAL "tsan")
    target_compile_options(${target_name} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
    target_link_options(${target_name} PRIVATE -fsanitize=thread)
  elseif (CLEARSPACE_SANITIZER STREQUAL "ubsan")
    target_compile_options(${target_name} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(${target_name} PRIVATE -fsanitize=undefined)
  else()
    message(FATAL_ERROR "Unknown CLEARSPACE_SANITIZER: ${CLEARSPACE_SANITIZER}")
  endif()
endfunction()

add_subdirectory(core)
add_subdirectory(persistence)
add_subdirectory(transport)
add_subdirectory(server)
add_subdirectory(app)

if (CLEARSPACE_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
