include(ExternalProject)

function(clearspace_prepare_static_qt)
  set(qt_src_dir "${CMAKE_BINARY_DIR}/_deps/qt-src")

  if (EXISTS "${CLEARSPACE_QT_STATIC_INSTALL_DIR}/lib/cmake/Qt6/Qt6Config.cmake")
    message(STATUS "Using existing static Qt from ${CLEARSPACE_QT_STATIC_INSTALL_DIR}")
    return()
  endif()

  if (WIN32)
    set(qt_platform_args -static -release -opensource -confirm-license -nomake examples -nomake tests)
  elseif(APPLE)
    set(qt_platform_args -static -release -opensource -confirm-license -nomake examples -nomake tests)
  else()
    set(qt_platform_args -static -release -opensource -confirm-license -nomake examples -nomake tests)
  endif()

  ExternalProject_Add(clearspace_qt_static
    URL "https://download.qt.io/archive/qt/${CLEARSPACE_QT_VERSION}/single/qt-everywhere-src-${CLEARSPACE_QT_VERSION}.tar.xz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR "${qt_src_dir}"
    BINARY_DIR "${CLEARSPACE_QT_STATIC_BUILD_DIR}"
    INSTALL_DIR "${CLEARSPACE_QT_STATIC_INSTALL_DIR}"
    CONFIGURE_COMMAND
      "${qt_src_dir}/configure"
      -prefix <INSTALL_DIR>
      -qt-zlib
      -qt-pcre
      -qt-libpng
      -qt-libjpeg
      ${qt_platform_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    BUILD_BYPRODUCTS "${CLEARSPACE_QT_STATIC_INSTALL_DIR}/lib/cmake/Qt6/Qt6Config.cmake"
  )

  set(CLEARSPACE_QT_STATIC_TARGET clearspace_qt_static PARENT_SCOPE)
endfunction()
