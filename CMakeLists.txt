cmake_minimum_required(VERSION 3.16)

project(FootballManagerGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB FM_ROOT_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE FM_SRC_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
set(FM_COMMON_SOURCES ${FM_ROOT_SOURCES} ${FM_SRC_SOURCES})

if(NOT WIN32)
    list(FILTER FM_COMMON_SOURCES EXCLUDE REGEX ".*/gui\\.cpp$")
endif()

if(WIN32)
    add_executable(FootballManager WIN32
        ${FM_COMMON_SOURCES}
    )
    target_link_libraries(FootballManager PRIVATE comctl32 gdi32)
else()
    add_executable(FootballManager
        ${FM_COMMON_SOURCES}
    )
endif()

target_include_directories(FootballManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(FootballManager PRIVATE /W4)
else()
    target_compile_options(FootballManager PRIVATE -Wall -Wextra -Wpedantic)
endif()
