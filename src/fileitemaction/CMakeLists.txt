set(part fileitemaction)

# kcoreaddons_add_plugin 需要 CMAKE_LIBRARY_OUTPUT_DIRECTORY（通常由
# KDECMakeSettings 设置），此处显式指定以满足宏的要求。
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(KF6CoreAddons REQUIRED)
find_package(KF6I18n REQUIRED)
find_package(KF6KIO REQUIRED)

kcoreaddons_add_plugin(${GLOBAL_PROJECT_NAME}_${part}
    SOURCES
        fileitemaction.cpp
        fileitemaction.h
    INSTALL_NAMESPACE kf6/kfileitemaction
)

target_link_libraries(${GLOBAL_PROJECT_NAME}_${part}
    PRIVATE
        ${GLOBAL_PROJECT_NAME}_core
        Qt6::Widgets
        KF6::I18n
        KF6::KIOWidgets
)

target_include_directories(${GLOBAL_PROJECT_NAME}_${part}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/core
)

defineProjectInfo(${part})
