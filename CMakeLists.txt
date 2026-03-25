cmake_minimum_required(VERSION 3.25)

project(
    Vanguard8
    VERSION 0.1.0
    DESCRIPTION "Vanguard 8 emulator scaffold"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

add_subdirectory(third_party)
add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
