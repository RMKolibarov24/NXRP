cmake_minimum_required(VERSION 3.28)

project(NXRPMaze LANGUAGES CXX)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(SFML)

add_executable(Maze sfml_test.cpp)

target_compile_features(Maze PRIVATE cxx_std_17)
target_link_libraries(Maze PRIVATE SFML::Graphics)