cmake_minimum_required(VERSION 4.1.2)

if(NOT DEFINED GLOBAL_PROJECT_NAME)
    set(GLOBAL_PROJECT_NAME "error_to_load")
endif()
if(NOT DEFINED GLOBAL_PROJECT_VERSION)
    set(GLOBAL_PROJECT_VERSION "0.0.0")
endif()
if(NOT DEFINED GLOBAL_PROJECT_HOMEPAGE)
    set(GLOBAL_PROJECT_HOMEPAGE "https://example.com")
endif()

project(${GLOBAL_PROJECT_NAME}
    LANGUAGES CXX
    VERSION ${GLOBAL_PROJECT_VERSION}
    HOMEPAGE_URL ${GLOBAL_PROJECT_HOMEPAGE}
)

# 让 kcoreaddons_add_plugin 生成可加载的共享模块插件（默认 OFF 会编成静态库且不安装）。
set(BUILD_SHARED_LIBS ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup(REQUIRES 6.10)
find_package(KF6CoreAddons)
find_package(KF6I18n)

add_subdirectory(src)
add_subdirectory(po)
