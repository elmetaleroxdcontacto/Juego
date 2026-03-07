cmake_minimum_required(VERSION 3.16)

project(FootballManagerGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FM_CAREER_SOURCES
    src/career/app_services.cpp
    src/career/career_support.cpp
)

set(FM_COMPETITION_SOURCES
    src/competition/competition.cpp
)

set(FM_ENGINE_SOURCES
    src/engine/models.cpp
)

set(FM_IO_SOURCES
    src/io/io.cpp
)

set(FM_SIMULATION_SOURCES
    src/simulation/match_events.cpp
    src/simulation/match_tactics.cpp
    src/simulation/simulation.cpp
)

set(FM_TRANSFER_SOURCES
    src/transfers/negotiation_system.cpp
)

set(FM_UI_SOURCES
    src/main.cpp
    src/ui/ui.cpp
    src/utils/utils.cpp
    src/validators/validators.cpp
)

set(FM_GUI_SOURCES)
if(WIN32)
    list(APPEND FM_GUI_SOURCES src/gui/gui.cpp)
endif()

add_executable(FootballManager
    ${FM_CAREER_SOURCES}
    ${FM_COMPETITION_SOURCES}
    ${FM_ENGINE_SOURCES}
    ${FM_IO_SOURCES}
    ${FM_SIMULATION_SOURCES}
    ${FM_TRANSFER_SOURCES}
    ${FM_UI_SOURCES}
    ${FM_GUI_SOURCES}
)

target_include_directories(FootballManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(FootballManager PRIVATE comctl32 gdi32)
endif()

if(MSVC)
    target_compile_options(FootballManager PRIVATE /W4)
else()
    target_compile_options(FootballManager PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(FootballManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
