function(complete name)
    target_compile_definitions(${GLOBAL_PROJECT_NAME}_${name} PRIVATE
        _PROJECT_NAME="${GLOBAL_PROJECT_NAME}"
        _PROJECT_VERSION="${GLOBAL_PROJECT_VERSION}"
        _PROJECT_HOMEPAGE="${GLOBAL_PROJECT_HOMEPAGE}"
    )

    install(TARGETS ${GLOBAL_PROJECT_NAME}_${name}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endfunction()

add_subdirectory(cli)
add_subdirectory(core)