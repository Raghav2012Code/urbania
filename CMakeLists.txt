cmake_minimum_required(VERSION 3.15)
project(Urbania VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(raylib REQUIRED)

add_executable(Urbania src/main.cpp src/core/Game.cpp src/core/Camera.cpp src/core/Input.cpp src/world/Tile.cpp src/world/World.cpp src/simulation/Economy.cpp src/simulation/SimulationClock.cpp)

target_include_directories(Urbania PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(Urbania PRIVATE raylib)

target_compile_options(Urbania PRIVATE -Wall -Wextra -Wpedantic)
