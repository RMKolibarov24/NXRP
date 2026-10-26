cmake_minimum_required(VERSION 3.28)
project(NXRPDependencies LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SFML)

# Only third-party dependencies use CMake. The four Maze projects are maintained
# directly in Visual Studio and import these generated library settings.
FetchContent_GetProperties(SFML SOURCE_DIR NXRP_SFML_SOURCE_DIR)
configure_file(SFML.props.in "${CMAKE_CURRENT_BINARY_DIR}/SFML.props.in" @ONLY)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/NXRP.SFML.$<CONFIG>.props"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/SFML.props.in")
