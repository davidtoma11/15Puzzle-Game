cmake_minimum_required(VERSION 3.10)
project(Game15 C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_search_module(FREETYPE REQUIRED freetype2)
find_package(X11 REQUIRED)

include_directories(
        ${X11_INCLUDE_DIR}
        ${FREETYPE_INCLUDE_DIRS}
        /usr/include
        /usr/include/freetype2
)

add_executable(Game15 15game.c)

target_link_libraries(Game15
        ${X11_LIBRARIES}
        Xft
        ${FREETYPE_LIBRARIES}
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(Game15 PRIVATE -g -O0)
endif()