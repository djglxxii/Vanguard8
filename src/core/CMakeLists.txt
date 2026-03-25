add_library(
    vanguard8_core
    bus.cpp
    emulator.cpp
    logging.cpp
    config.cpp
    memory/cartridge.cpp
    memory/sram.cpp
)

target_include_directories(vanguard8_core PUBLIC ${PROJECT_SOURCE_DIR}/src)

find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
    target_link_libraries(vanguard8_core PUBLIC spdlog::spdlog)
    target_compile_definitions(vanguard8_core PUBLIC VANGUARD8_HAS_SPDLOG=1)
endif()
