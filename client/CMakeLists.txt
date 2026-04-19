set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Threads REQUIRED)

qt_add_executable(clearspace_qt_client
        MANUAL_FINALIZATION
        src/main.cpp

        src/network/client_protocol.cpp
        src/network/network_manager.cpp
        src/model/client_context.cpp
        src/model/board_model.cpp
        src/ui/login_window.cpp
        src/ui/room_selection_window.cpp
        src/ui/main_window.cpp
        src/ui/canvas_widget.cpp

        include/clearspace/client/network/client_protocol.hpp
        include/clearspace/client/network/network_manager.hpp
        include/clearspace/client/model/client_context.hpp
        include/clearspace/client/model/board_model.hpp
        include/clearspace/client/ui/login_window.hpp
        include/clearspace/client/ui/room_selection_window.hpp
        include/clearspace/client/ui/main_window.hpp
        include/clearspace/client/ui/canvas_widget.hpp
)

set_target_properties(clearspace_qt_client PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

if (DEFINED CLEARSPACE_QT_STATIC_TARGET)
  add_dependencies(clearspace_qt_client ${CLEARSPACE_QT_STATIC_TARGET})
endif()

target_include_directories(clearspace_qt_client
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(clearspace_qt_client PRIVATE cxx_std_20)

target_link_libraries(clearspace_qt_client
        PRIVATE
        clearspace_transport
        clearspace_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Boost::headers
        nlohmann_json::nlohmann_json
        Threads::Threads
)

if (WIN32)
  target_link_libraries(clearspace_qt_client PRIVATE ws2_32)
endif()

target_compile_definitions(clearspace_qt_client
        PRIVATE
        BOOST_ASIO_DISABLE_LIB
        BOOST_ERROR_CODE_HEADER_ONLY
)

clearspace_enable_warnings(clearspace_qt_client)
clearspace_enable_sanitizer(clearspace_qt_client)

if (COMMAND qt_finalize_executable AND NOT APPLE)
  qt_finalize_executable(clearspace_qt_client)
endif()

if (CLEARSPACE_ENABLE_INSTALL)
  install(TARGETS clearspace_qt_client
          BUNDLE DESTINATION .
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  if (WIN32 AND CLEARSPACE_QT_MODE STREQUAL "deploy" AND COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
            TARGET clearspace_qt_client
            OUTPUT_SCRIPT clearspace_qt_deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${clearspace_qt_deploy_script})
  endif()
endif()