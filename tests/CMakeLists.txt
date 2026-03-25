include(FetchContent)

find_package(Catch2 3 CONFIG QUIET)
if(NOT Catch2_FOUND)
    FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
    )
    FetchContent_MakeAvailable(Catch2)
endif()

add_executable(vanguard8_tests catch_main.cpp test_scaffold.cpp)
target_link_libraries(vanguard8_tests PRIVATE Catch2::Catch2 vanguard8_core)
target_include_directories(vanguard8_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(Catch)
catch_discover_tests(vanguard8_tests)
