cmake_minimum_required(VERSION 3.24)

if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(clearspace
        VERSION 0.3.0
        DESCRIPTION "ClearSpace collaborative whiteboard"
        LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(CMakeDependentOption)

option(CLEARSPACE_ENABLE_TESTS "Build tests" ON)
option(CLEARSPACE_ENABLE_QT_CLIENT "Build Qt client" ON)
option(CLEARSPACE_BUILD_SHARED_LIBS "Build ClearSpace libraries as shared" OFF)
option(CLEARSPACE_ENABLE_INSTALL "Enable install rules" ON)
option(CLEARSPACE_ENABLE_PACKAGING "Enable CPack package generation" ON)

set(BUILD_SHARED_LIBS ${CLEARSPACE_BUILD_SHARED_LIBS})

set(CLEARSPACE_DEPS_MODE "auto" CACHE STRING "Dependency mode: auto|system|fetch")
set_property(CACHE CLEARSPACE_DEPS_MODE PROPERTY STRINGS auto system fetch)

set(CLEARSPACE_QT_MODE "deploy" CACHE STRING "Qt delivery mode: deploy|system-only|static-source")
set_property(CACHE CLEARSPACE_QT_MODE PROPERTY STRINGS deploy system-only static-source)

set(CLEARSPACE_QT_VERSION "6.8.2" CACHE STRING "Qt version used for static-source builds")
set(CLEARSPACE_QT_STATIC_INSTALL_DIR "${CMAKE_BINARY_DIR}/qt-static" CACHE PATH "Where static Qt should be installed")
set(CLEARSPACE_QT_STATIC_BUILD_DIR "${CMAKE_BINARY_DIR}/qt-build" CACHE PATH "Where static Qt should be built")

set(CLEARSPACE_SANITIZER "none" CACHE STRING "Sanitizer: none|asan|tsan|ubsan")
set_property(CACHE CLEARSPACE_SANITIZER PROPERTY STRINGS none asan tsan ubsan)

include(ClearSpaceWarnings)
include(ClearSpaceDependencies)

clearspace_setup_common_dependencies()

add_subdirectory(core)
add_subdirectory(persistence)
add_subdirectory(transport)
add_subdirectory(server)
add_subdirectory(app)

if (CLEARSPACE_ENABLE_QT_CLIENT)
  add_subdirectory(client)
endif()

if (CLEARSPACE_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()


if (CLEARSPACE_ENABLE_PACKAGING)
  set(CPACK_PACKAGE_NAME "ClearSpace")
  set(CPACK_PACKAGE_VENDOR "OpenAI")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ClearSpace collaborative whiteboard")
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "ClearSpace")
  set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
  set(CPACK_GENERATOR "ZIP;TGZ")
  include(CPack)
endif()
