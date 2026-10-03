cmake_minimum_required(VERSION 3.15)
project(Urbania VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(raylib REQUIRED)

# Shared simulation/world sources. Single source of truth so the game and
# all unit tests always build the same logic (no duplicated file lists).
set(URBANIA_SIM_SOURCES
    src/world/Tile.cpp
    src/world/World.cpp
    src/simulation/Economy.cpp
    src/simulation/SimulationClock.cpp
    src/simulation/Simulation.cpp
    src/simulation/Population.cpp
    src/simulation/Citizen.cpp
    src/simulation/CitizenManager.cpp
    src/simulation/Employment.cpp
    src/simulation/RoadNetwork.cpp
    src/simulation/Pathfinder.cpp
    src/simulation/CommuteSystem.cpp
    src/simulation/CitizenMovement.cpp
    src/simulation/Vehicle.cpp
    src/simulation/Traffic.cpp
    src/simulation/Congestion.cpp
    src/simulation/Demand.cpp
    src/simulation/Pollution.cpp
    src/simulation/Happiness.cpp
    src/simulation/LandValue.cpp
    src/simulation/Housing.cpp
    src/simulation/Utilities.cpp
    src/simulation/Bus.cpp
    src/simulation/BusStop.cpp
    src/simulation/BusRoute.cpp
    src/simulation/Transit.cpp
)

add_library(urbania_sim STATIC ${URBANIA_SIM_SOURCES})
target_include_directories(urbania_sim PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(urbania_sim PUBLIC raylib)
target_compile_options(urbania_sim PRIVATE -Wall -Wextra -Wpedantic)

add_executable(Urbania
    src/main.cpp
    src/core/Game.cpp
    src/core/SaveSystem.cpp
    src/core/SelfTest.cpp
    src/core/Camera.cpp
    src/core/Input.cpp
    src/rendering/TextureManager.cpp
    src/rendering/TileRenderer.cpp
    src/rendering/EntityRenderer.cpp
    src/ui/UI.cpp
)

target_include_directories(Urbania PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(Urbania PRIVATE urbania_sim raylib)
target_compile_options(Urbania PRIVATE -Wall -Wextra -Wpedantic)

add_executable(test_utilities
    tests/test_utilities.cpp
)
target_include_directories(test_utilities PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_utilities PRIVATE urbania_sim raylib)
target_compile_options(test_utilities PRIVATE -Wall -Wextra -Wpedantic)

add_executable(test_simulation_balance
    tests/test_simulation_balance.cpp
)
target_include_directories(test_simulation_balance PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_simulation_balance PRIVATE urbania_sim raylib)
target_compile_options(test_simulation_balance PRIVATE -Wall -Wextra -Wpedantic)

# Register the suites with CTest so `ctest` (and CI) can run them.
enable_testing()
add_test(NAME utilities COMMAND test_utilities)
add_test(NAME simulation_balance COMMAND test_simulation_balance)
