find_package(OpenGL REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_library(
    vanguard8_frontend_lib
    app.cpp
    audio_output.cpp
    display.cpp
    display_presenter.cpp
    input.cpp
    rom_loader.cpp
    runtime.cpp
    sdl_window.cpp
    video_fixture.cpp
)
target_include_directories(vanguard8_frontend_lib PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(
    vanguard8_frontend_lib
    PUBLIC
        OpenGL::GL
        SDL2::SDL2
        vanguard8_core
        vanguard8_debugger
)

add_library(vanguard8_headless_lib headless.cpp display.cpp input.cpp rom_loader.cpp video_fixture.cpp)
target_include_directories(vanguard8_headless_lib PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(vanguard8_headless_lib PUBLIC vanguard8_core)
