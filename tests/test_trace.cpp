#include <catch2/catch_test_macros.hpp>

#include "core/emulator.hpp"
#include "core/memory/cartridge.hpp"
#include "debugger/trace_panel.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

TEST_CASE("trace writer persists decoded instruction history to a file", "[debugger]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x3E;
    rom[0x0001] = 0x42;
    rom[0x0002] = 0x21;
    rom[0x0003] = 0x34;
    rom[0x0004] = 0x12;
    rom[0x0005] = 0xED;
    rom[0x0006] = 0x47;
    rom[0x0007] = 0x76;

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);

    const auto trace_path =
        std::filesystem::temp_directory_path() / "vanguard8-trace-writer-test.log";
    std::filesystem::remove(trace_path);

    vanguard8::debugger::TracePanel panel;
    const auto result = panel.write_to_file(emulator, trace_path, 8);

    REQUIRE(result.line_count == 4);
    REQUIRE(result.halted);

    std::ifstream input(trace_path);
    REQUIRE(input.good());
    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    REQUIRE(contents.find("STEP         PC    PHYS   BANK  BYTES        MNEMONIC") != std::string::npos);
    REQUIRE(contents.find("000000000000  0000  00000     F  3E 42        LD A,0x42") != std::string::npos);
    REQUIRE(contents.find("000000000001  0002  00002     F  21 34 12     LD HL,0x1234") != std::string::npos);
    REQUIRE(contents.find("000000000002  0005  00005     F  ED 47        LD I,A") != std::string::npos);
    REQUIRE(contents.find("000000000003  0007  00007     F  76           HALT") != std::string::npos);

    std::filesystem::remove(trace_path);
}

TEST_CASE("trace runtime summary reports the narrow bring-up status surface", "[debugger]") {
    vanguard8::core::Emulator emulator;
    emulator.set_vclk_rate(vanguard8::core::VclkRate::hz_6000);
    emulator.run_frames(1);

    const auto summary = vanguard8::debugger::format_trace_runtime_summary(emulator, "fixture mode");

    REQUIRE(summary.find("Runtime status: fixture mode") != std::string::npos);
    REQUIRE(summary.find("Frame count: 1") != std::string::npos);
    REQUIRE(summary.find("VCLK rate: 6000") != std::string::npos);
    REQUIRE(summary.find("Event log size:") != std::string::npos);
    REQUIRE(summary.find("Loaded ROM size:") != std::string::npos);
}
