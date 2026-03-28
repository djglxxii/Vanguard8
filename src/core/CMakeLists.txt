add_library(
    vanguard8_core
    bus.cpp
    emulator.cpp
    hash.cpp
    logging.cpp
    rewind.cpp
    save_state.cpp
    scheduler.cpp
    symbols.cpp
    config.cpp
    audio/audio_mixer.cpp
    audio/ay8910.cpp
    audio/ayumi_vendor.cpp
    audio/msm5205_adapter.cpp
    audio/opm_vendor.cpp
    audio/ym2151.cpp
    io/controller.cpp
    replay/recorder.cpp
    replay/replayer.cpp
    video/v9938.cpp
    cpu/z180_adapter.cpp
    ../../third_party/msm5205/msm5205_core.cpp
    ../../third_party/z180/z180_core.cpp
    memory/cartridge.cpp
    memory/sram.cpp
)

target_include_directories(vanguard8_core PUBLIC ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})

find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
    target_link_libraries(vanguard8_core PUBLIC spdlog::spdlog)
    target_compile_definitions(vanguard8_core PUBLIC VANGUARD8_HAS_SPDLOG=1)
endif()
