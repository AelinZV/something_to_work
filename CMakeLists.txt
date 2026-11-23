cmake_minimum_required(VERSION 3.16)
project(BorderlandContracts VERSION 1.5 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(borderland_core STATIC
    src/domain/World.cpp
    src/service/GameService.cpp
    src/repository/MemoryGameRepository.cpp)
target_include_directories(borderland_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(MSVC)
    target_compile_options(borderland_core PRIVATE /W4)
else()
    target_compile_options(borderland_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# PostgreSQL/libpqxx: реальный режим, если библиотека установлена.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PQXX QUIET IMPORTED_TARGET libpqxx)
endif()
if(NOT PQXX_FOUND)
    find_path(PQXX_INCLUDE_DIR NAMES pqxx/pqxx)
    find_library(PQXX_LIBRARY NAMES pqxx)
    if(PQXX_INCLUDE_DIR AND PQXX_LIBRARY)
        set(PQXX_FOUND TRUE)
    endif()
endif()

add_library(borderland_app STATIC src/app/RepositoryFactory.cpp)
target_link_libraries(borderland_app PUBLIC borderland_core)
target_include_directories(borderland_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(PQXX_FOUND)
    target_sources(borderland_app PRIVATE src/repository/PgGameRepository.cpp)
    target_compile_definitions(borderland_app PUBLIC BORDERLAND_HAVE_PQXX=1)
    if(TARGET PkgConfig::PQXX)
        target_link_libraries(borderland_app PUBLIC PkgConfig::PQXX)
    else()
        target_include_directories(borderland_app PRIVATE ${PQXX_INCLUDE_DIR})
        target_link_libraries(borderland_app PUBLIC ${PQXX_LIBRARY})
    endif()
    message(STATUS "PostgreSQL storage: ENABLED (libpqxx found)")
else()
    message(WARNING "libpqxx not found: PostgreSQL mode unavailable; --demo and tests still build")
endif()

add_executable(borderland_cli src/cli/main.cpp)
target_link_libraries(borderland_cli PRIVATE borderland_app)

option(BORDERLAND_BUILD_GUI "Build Qt 6 graphical interface when installed" ON)
if(BORDERLAND_BUILD_GUI)
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if(TARGET Qt6::Widgets)
        add_executable(borderland_gui src/gui/main.cpp)
        target_link_libraries(borderland_gui PRIVATE borderland_app Qt6::Widgets)
        target_compile_definitions(borderland_gui PRIVATE BORDERLAND_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
        message(STATUS "Qt 6 Widgets graphical frontend: ENABLED")
    else()
        message(WARNING "Qt6 Widgets not found: GUI disabled. Ubuntu/WSL: sudo apt install qt6-base-dev")
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(borderland_tests tests/GameTests.cpp)
    target_link_libraries(borderland_tests PRIVATE borderland_core)
    set(GAME_TESTS
        create_hero reject_empty_name separate_saves accept_order
        investigate_requires_travel investigate_choice travel_during_battle
        starting_battle combat_survives_reload potion_heals oil_consumption
        crafting_uses_herbs insufficient_funds cannot_accept_twice
        cannot_investigate_twice flee_resets_battle defeat_revives
        victory_rewards no_double_reward ritual_branch level_up
        story_completed delete_hero inventory_persists
        skill_points_on_level skill_melee skill_vitality skill_sign skill_saved
        skill_without_points skill_invalid_choice rest_heals rest_invalid_location
        rest_full_health potion_outside_battle potion_full_health
        rest_during_battle skill_during_battle potion_during_battle_rejected
        multiple_contracts_one_location new_contract_existing_hero
        ritual_new_ghost_contract reject_duplicate_contract_id
        responsive_layout_minimum responsive_layout_regular responsive_layout_wide
        map_status_markers map_multiple_contracts_same_status
        map_markers_follow_progress map_invalid_quest_data
    )
    foreach(t IN LISTS GAME_TESTS)
        add_test(NAME ${t} COMMAND borderland_tests ${t})
    endforeach()
    if(TARGET borderland_gui)
        add_test(NAME qt_gui_offscreen_smoke COMMAND borderland_gui --demo --smoke-test)
        set_tests_properties(qt_gui_offscreen_smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()
