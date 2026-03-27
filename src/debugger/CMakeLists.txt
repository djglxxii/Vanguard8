add_library(
    vanguard8_debugger
    debugger.cpp
    cpu_panel.cpp
    memory_panel.cpp
    vdp_panel.cpp
)
target_include_directories(vanguard8_debugger PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(vanguard8_debugger PUBLIC vanguard8_core)
