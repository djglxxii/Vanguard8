include(FetchContent)

find_package(OpenGL REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

find_package(Catch2 3 CONFIG QUIET)
if(NOT Catch2_FOUND)
    FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
    )
    FetchContent_MakeAvailable(Catch2)
endif()

add_executable(
    vanguard8_tests
    catch_main.cpp
    test_audio.cpp
    test_rewind_replay.cpp
    test_scaffold.cpp
    test_save_state.cpp
    test_debugger.cpp
    test_debugger_logs.cpp
    test_debugger_panels.cpp
    test_bus.cpp
    test_cpu.cpp
    test_frontend_backends.cpp
    test_frontend_runtime.cpp
    test_input.cpp
    test_integration.cpp
    test_scheduler.cpp
    test_video.cpp
    ../src/frontend/audio_output.cpp
    ../src/frontend/display.cpp
    ../src/frontend/display_presenter.cpp
    ../src/frontend/input.cpp
    ../src/frontend/rom_loader.cpp
    ../src/frontend/runtime.cpp
    ../src/frontend/sdl_window.cpp
    ../src/frontend/video_fixture.cpp
)
target_link_libraries(vanguard8_tests PRIVATE Catch2::Catch2 vanguard8_core vanguard8_debugger)
target_link_libraries(vanguard8_tests PRIVATE OpenGL::GL SDL2::SDL2)
target_include_directories(vanguard8_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(Catch)
catch_discover_tests(vanguard8_tests)

add_test(
    NAME vanguard8_headless_replay_regression
    COMMAND
        $<TARGET_FILE:vanguard8_headless>
        --rom ${PROJECT_SOURCE_DIR}/tests/replays/replay_fixture.rom
        --replay ${PROJECT_SOURCE_DIR}/tests/replays/replay_fixture.v8r
        --hash-frame 4
        --expect-frame-hash 4 e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c
        --hash-audio
        --expect-audio-hash 48beda9f68f15ac4e3fca3f8b54ebea7832a906d358cc49867ae508287039ddf
)
set_tests_properties(
    vanguard8_headless_replay_regression
    PROPERTIES ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg-config"
)
