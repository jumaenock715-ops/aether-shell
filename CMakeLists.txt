cmake_minimum_required(VERSION 3.21)
project(AetherShell VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick DBus)
qt_standard_project_setup(REQUIRES 6.5)

# Window observation is optional at build time. Aether remains usable when
# building without Wayland development headers or the protocol scanner.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(WAYLAND_CLIENT QUIET IMPORTED_TARGET wayland-client)
endif()
find_program(WAYLAND_SCANNER wayland-scanner)

set(AETHER_HAVE_WAYLAND 0)
if(WAYLAND_CLIENT_FOUND AND WAYLAND_SCANNER)
    set(AETHER_HAVE_WAYLAND 1)
    set(FOREIGN_TOPLEVEL_XML
        ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-foreign-toplevel-list-v1.xml)
    set(FOREIGN_TOPLEVEL_HEADER
        ${CMAKE_CURRENT_BINARY_DIR}/ext-foreign-toplevel-list-v1-client-protocol.h)
    set(FOREIGN_TOPLEVEL_CODE
        ${CMAKE_CURRENT_BINARY_DIR}/ext-foreign-toplevel-list-v1-protocol.c)

    add_custom_command(
        OUTPUT ${FOREIGN_TOPLEVEL_HEADER}
        COMMAND ${WAYLAND_SCANNER} client-header
                ${FOREIGN_TOPLEVEL_XML} ${FOREIGN_TOPLEVEL_HEADER}
        DEPENDS ${FOREIGN_TOPLEVEL_XML}
        VERBATIM)
    add_custom_command(
        OUTPUT ${FOREIGN_TOPLEVEL_CODE}
        COMMAND ${WAYLAND_SCANNER} private-code
                ${FOREIGN_TOPLEVEL_XML} ${FOREIGN_TOPLEVEL_CODE}
        DEPENDS ${FOREIGN_TOPLEVEL_XML}
        VERBATIM)
endif()

qt_add_executable(aether-shell
    main.cpp
    ApplicationService.cpp
    ApplicationService.h
    AndroidRuntimeAdapter.h
    AndroidSessionManager.cpp
    AndroidSessionManager.h
    WaydroidRuntimeAdapter.cpp
    WaydroidRuntimeAdapter.h
    WaylandWindowTracker.cpp
    WaylandWindowTracker.h
    NotificationService.cpp
    NotificationService.h
    NotificationDBusAdaptor.cpp
    NotificationDBusAdaptor.h
    AudioService.cpp
    AudioService.h
    WaydroidBridgeService.cpp
    WaydroidBridgeService.h
    ClipboardService.cpp
    ClipboardService.h
    FileService.cpp
    FileService.h
    SessionManager.cpp
    SessionManager.h
    SessionSettings.cpp
    SessionSettings.h
    AppCatalog.cpp
    AppCatalog.h
)

# Singletons must be flagged before the QML module is created.
set_source_files_properties(
    Theme.qml Apps.qml System.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(aether-shell
    URI Aether
    VERSION 1.0
    QML_FILES
        Main.qml
        Theme.qml
        Apps.qml
        System.qml
        AppIcon.qml
        Toggle.qml
        AetherSlider.qml
        QuickTile.qml
        TopBar.qml
        Dock.qml
        Launcher.qml
        QuickSettings.qml
        NotificationCenter.qml
        AppWindow.qml
        FilesApp.qml
        SettingsApp.qml
        PlaceholderApp.qml
)

target_compile_definitions(aether-shell PRIVATE AETHER_HAVE_WAYLAND=${AETHER_HAVE_WAYLAND})
if(AETHER_HAVE_WAYLAND)
    target_sources(aether-shell PRIVATE
        ${FOREIGN_TOPLEVEL_HEADER}
        ${FOREIGN_TOPLEVEL_CODE})
    target_include_directories(aether-shell PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(aether-shell PRIVATE PkgConfig::WAYLAND_CLIENT)
endif()

target_link_libraries(aether-shell PRIVATE Qt6::Quick Qt6::DBus)
