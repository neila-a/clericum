set(part cli)

qt_add_executable(${GLOBAL_PROJECT_NAME}_${part}
    main.cpp
    commandparser.cpp
)

target_link_libraries(${GLOBAL_PROJECT_NAME}_${part}
    PRIVATE
        KF6::CoreAddons
        ${GLOBAL_PROJECT_NAME}_core
)

target_include_directories(${GLOBAL_PROJECT_NAME}_${part}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/core
)

complete(${part})