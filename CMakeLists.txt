cmake_minimum_required(VERSION 3.21)
project(AetherShell VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(aether-shell
    main.cpp
    ApplicationService.cpp
    ApplicationService.h
    NotificationService.cpp
    NotificationService.h
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
        SettingsApp.qml
        PlaceholderApp.qml
)

target_link_libraries(aether-shell PRIVATE Qt6::Quick)
