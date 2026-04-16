cmake_minimum_required(VERSION 3.16)

project(clericum VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(clericum_app
    main.cpp
)

qt_add_qml_module(clericum_app
    URI clericum
    QML_FILES
        Main.qml
)

target_link_libraries(clericum_app
    PRIVATE Qt6::Quick
)

install(TARGETS clericum_app
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
