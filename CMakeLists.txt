cmake_minimum_required(VERSION 3.16)

project(FootballManagerGame LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FM_CAREER_SOURCES
    src/career/app_services.cpp
    src/career/career_runtime.cpp
    src/career/career_reports.cpp
    src/career/career_manager.cpp
    src/career/season_flow_controller.cpp
    src/career/season_service.cpp
    src/career/season_transition.cpp
    src/career/career_support.cpp
    src/career/player_development.cpp
    src/career/team_management.cpp
    src/career/week_simulation.cpp
)

set(FM_AI_SOURCES
    src/ai/ai_match_manager.cpp
    src/ai/ai_squad_planner.cpp
    src/ai/team_ai.cpp
    src/ai/ai_transfer_manager.cpp
)

set(FM_COMPETITION_SOURCES
    src/competition/competition.cpp
)

set(FM_ENGINE_SOURCES
    src/engine/career_state.cpp
    src/engine/game_engine.cpp
    src/engine/models.cpp
)

set(FM_APP_SOURCES
    src/engine/game_controller.cpp
)

set(FM_IO_SOURCES
    src/io/io.cpp
    src/io/save_manager.cpp
    src/io/save_serialization.cpp
)

set(FM_SIMULATION_SOURCES
    src/simulation/fatigue_engine.cpp
    src/simulation/match_event_generator.cpp
    src/simulation/match_events.cpp
    src/simulation/match_context.cpp
    src/simulation/match_engine.cpp
    src/simulation/match_phase.cpp
    src/simulation/match_postprocess.cpp
    src/simulation/match_report.cpp
    src/simulation/match_resolution.cpp
    src/simulation/match_stats.cpp
    src/simulation/match_tactics.cpp
    src/simulation/morale_engine.cpp
    src/simulation/player_condition.cpp
    src/simulation/simulation.cpp
    src/simulation/tactics_engine.cpp
)

set(FM_TRANSFER_SOURCES
    src/transfers/negotiation_system.cpp
    src/transfers/transfer_market.cpp
)

set(FM_DEVELOPMENT_SOURCES
    src/development/player_progression_system.cpp
    src/development/training_impact_system.cpp
    src/development/youth_generation_system.cpp
)

set(FM_FINANCE_SOURCES
    src/finance/finance_system.cpp
)

set(FM_SUPPORT_SOURCES
    src/utils/utils.cpp
)

set(FM_VALIDATOR_SOURCES
    src/validators/validators.cpp
)

set(FM_CORE_SOURCES
    ${FM_CAREER_SOURCES}
    ${FM_AI_SOURCES}
    ${FM_COMPETITION_SOURCES}
    ${FM_ENGINE_SOURCES}
    ${FM_IO_SOURCES}
    ${FM_SIMULATION_SOURCES}
    ${FM_TRANSFER_SOURCES}
    ${FM_DEVELOPMENT_SOURCES}
    ${FM_FINANCE_SOURCES}
    ${FM_SUPPORT_SOURCES}
)

set(FM_CONSOLE_UI_SOURCES
    src/ui/career_reports_ui.cpp
    src/ui/cup_ui.cpp
    src/ui/market_ui.cpp
    src/ui/ui.cpp
)

set(FM_ENTRY_SOURCES
    src/main.cpp
)

set(FM_GUI_SOURCES)
if(WIN32)
    list(APPEND FM_GUI_SOURCES
        src/gui/gui.cpp
        src/gui/gui_actions.cpp
        src/gui/gui_layout.cpp
        src/gui/gui_shared.cpp
        src/gui/gui_views.cpp
    )
endif()

add_executable(FootballManager
    ${FM_CORE_SOURCES}
    ${FM_APP_SOURCES}
    ${FM_ENTRY_SOURCES}
    ${FM_CONSOLE_UI_SOURCES}
    ${FM_VALIDATOR_SOURCES}
    ${FM_GUI_SOURCES}
)

function(fm_configure_target target_name)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    if(WIN32)
        target_link_libraries(${target_name} PRIVATE comctl32 gdi32)
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endfunction()

fm_configure_target(FootballManager)

if(BUILD_TESTING)
    add_executable(FootballManagerTests
        tests/project_tests.cpp
        ${FM_CORE_SOURCES}
        ${FM_VALIDATOR_SOURCES}
    )
    fm_configure_target(FootballManagerTests)
    add_test(NAME FootballManagerTests COMMAND FootballManagerTests)
    set_tests_properties(FootballManagerTests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
