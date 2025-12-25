cmake_minimum_required(VERSION 3.16)

project(MustangClock VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Bluetooth)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMustangClock
    main.cpp
    BleManager.cpp
    BleManager.h
)

qt_add_qml_module(appMustangClock
    URI MustangClock
    QML_FILES
        Main.qml
        AlarmBox.qml
        TimeBox.qml
        WifiBox.qml
)

target_link_libraries(appMustangClock
    PRIVATE
        Qt6::Quick
        Qt6::Bluetooth
)

set_target_properties(appMustangClock PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
