#include <catch2/catch_test_macros.hpp>

#include "core/emulator.hpp"
#include "core/memory/cartridge.hpp"
#include "core/symbols.hpp"
#include "debugger/cpu_panel.hpp"
#include "debugger/memory_panel.hpp"
#include "debugger/trace_panel.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

TEST_CASE("symbol table loads .sym content and formats nearest labels", "[debugger]") {
    vanguard8::core::SymbolTable symbols;
    symbols.load_from_string(
        "0000 reset_vector\n"
        "0038 int0_handler\n"
        "0050 main_loop\n"
        "; comment\n"
        "8000 sram_base\n"
    );

    REQUIRE(symbols.size() == 4);
    REQUIRE(symbols.find_exact(0x0038) != nullptr);
    REQUIRE(symbols.find_exact(0x0038)->label == "int0_handler");
    REQUIRE(symbols.find_by_name("main_loop") != nullptr);
    REQUIRE(symbols.find_by_name("main_loop")->address == 0x0050);
    REQUIRE(symbols.format_address(0x0050) == "main_loop");
    REQUIRE(symbols.format_address(0x0052) == "main_loop+0x2");
}

TEST_CASE("cpu and memory panel snapshots annotate addresses with loaded symbols", "[debugger]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0xCD;
    rom[0x0001] = 0x50;
    rom[0x0002] = 0x00;
    rom[0x0003] = 0x76;

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);

    vanguard8::core::SymbolTable symbols;
    symbols.load_from_string(
        "0000 reset_vector\n"
        "0050 main_loop\n"
        "8000 sram_base\n"
        "8001 player_x\n"
    );

    vanguard8::debugger::CpuPanel cpu_panel;
    const auto cpu_snapshot = cpu_panel.snapshot(emulator, symbols);
    REQUIRE(cpu_snapshot.disassembly[0].address_label == "reset_vector");
    REQUIRE(cpu_snapshot.disassembly[0].mnemonic == "CALL 0x0050  ; main_loop");

    vanguard8::debugger::MemoryPanel memory_panel;
    const auto memory_snapshot = memory_panel.snapshot(
        emulator,
        vanguard8::debugger::MemorySelection{
            .region = vanguard8::debugger::MemoryRegion::logical,
            .start = 0x8000,
            .length = 2,
        },
        symbols
    );
    REQUIRE(memory_snapshot.annotations.size() == 2);
    REQUIRE(memory_snapshot.annotations[0] == "sram_base");
    REQUIRE(memory_snapshot.annotations[1] == "player_x");
}

TEST_CASE("trace writer appends symbol annotations when a symbol table is loaded", "[debugger]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0xCD;
    rom[0x0001] = 0x50;
    rom[0x0002] = 0x00;
    rom[0x0050] = 0x76;

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);

    vanguard8::core::SymbolTable symbols;
    symbols.load_from_string(
        "0000 reset_vector\n"
        "0050 main_loop\n"
    );

    const auto trace_path =
        std::filesystem::temp_directory_path() / "vanguard8-symbol-trace-test.log";
    std::filesystem::remove(trace_path);

    vanguard8::debugger::TracePanel panel;
    const auto result = panel.write_to_file(emulator, trace_path, 4, &symbols);
    REQUIRE(result.line_count == 2);
    REQUIRE(result.halted);

    std::ifstream input(trace_path);
    REQUIRE(input.good());
    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    REQUIRE(contents.find("CALL 0x0050  ; main_loop  [reset_vector]") != std::string::npos);
    REQUIRE(contents.find("HALT") != std::string::npos);

    std::filesystem::remove(trace_path);
}
