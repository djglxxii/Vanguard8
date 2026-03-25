add_library(
    vanguard8_frontend_lib
    app.cpp
    display.cpp
    input.cpp
)
target_include_directories(vanguard8_frontend_lib PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(vanguard8_frontend_lib PUBLIC vanguard8_core vanguard8_debugger)

add_library(vanguard8_headless_lib headless.cpp)
target_include_directories(vanguard8_headless_lib PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(vanguard8_headless_lib PUBLIC vanguard8_core)
