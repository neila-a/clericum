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