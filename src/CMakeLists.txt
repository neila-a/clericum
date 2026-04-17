qt_standard_project_setup(REQUIRES 6.10)

# Dont remove `_app` suffix!

qt_add_executable(${GLOBAL_PROJECT_NAME}_app
    main.cpp
)

qt_add_qml_module(${GLOBAL_PROJECT_NAME}_app
    URI ${GLOBAL_PROJECT_NAME}
    QML_FILES
        Main.qml
)

target_link_libraries(${GLOBAL_PROJECT_NAME}_app
    PRIVATE Qt6::Quick
)

target_compile_definitions(${GLOBAL_PROJECT_NAME}_app PRIVATE
    _PROJECT_NAME="${GLOBAL_PROJECT_NAME}"
    _PROJECT_DESCRIPTION="${GLOBAL_PROJECT_DESCRIPTION}"
    _PROJECT_VERSION="${GLOBAL_PROJECT_VERSION}"
)

install(TARGETS ${GLOBAL_PROJECT_NAME}_app
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)