set(part core)

add_library(${GLOBAL_PROJECT_NAME}_${part} SHARED
    clericumfuse.cpp
    clericumfuse_operations.cpp
    commandhandler.cpp
    storemanager.cpp
)

target_link_libraries(${GLOBAL_PROJECT_NAME}_${part}
    PRIVATE
        fuse3
    PUBLIC
        Qt6::Core
        KF6::I18n
)

complete(${part})
