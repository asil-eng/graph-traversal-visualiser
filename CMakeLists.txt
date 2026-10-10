cmake_minimum_required(VERSION 3.16)
project(PathfindingVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)

add_executable(pathfinder
    src/main.cpp
    src/Grid.cpp
    src/Algorithms.cpp
    src/MazeGenerator.cpp
    src/EditHistory.cpp
    src/Benchmark.cpp
)

target_include_directories(pathfinder PRIVATE src)
target_link_libraries(pathfinder PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
