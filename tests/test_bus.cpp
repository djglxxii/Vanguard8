#include <catch2/catch_test_macros.hpp>

#include "core/bus.hpp"

#include <vector>

namespace {

auto make_pattern_rom(const std::size_t bytes) -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(bytes, 0xFF);
    for (std::size_t page = 0; page < (bytes / 0x4000); ++page) {
        const auto value = static_cast<std::uint8_t>(page & 0xFF);
        const auto start = page * 0x4000;
        const auto end = start + 0x4000;
        std::fill(rom.begin() + static_cast<std::ptrdiff_t>(start), rom.begin() + static_cast<std::ptrdiff_t>(end), value);
    }
    return rom;
}

}  // namespace

TEST_CASE("cartridge size validation enforces 960 KB max and 16 KB pages", "[bus]") {
    REQUIRE_NOTHROW(vanguard8::core::memory::CartridgeSlot(std::vector<std::uint8_t>(0x4000, 0xAA)));
    REQUIRE_NOTHROW(
        vanguard8::core::memory::CartridgeSlot(
            std::vector<std::uint8_t>(vanguard8::core::memory::CartridgeSlot::max_rom_size, 0x55)
        )
    );
    REQUIRE_THROWS_AS(
        vanguard8::core::memory::CartridgeSlot(std::vector<std::uint8_t>(0x2000, 0x00)),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        vanguard8::core::memory::CartridgeSlot(
            std::vector<std::uint8_t>(
                vanguard8::core::memory::CartridgeSlot::max_rom_size + 0x4000,
                0x00
            )
        ),
        std::invalid_argument
    );
}

TEST_CASE("fixed ROM region maps physical 0x00000-0x03FFF", "[bus]") {
    const auto rom = make_pattern_rom(0x8000);
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};

    REQUIRE(bus.read_memory(0x00000) == 0x00);
    REQUIRE(bus.read_memory(0x03FFF) == 0x00);
}

TEST_CASE("banked ROM region maps physical 0x04000-0xEFFFF", "[bus]") {
    const auto rom =
        make_pattern_rom(vanguard8::core::memory::CartridgeSlot::max_rom_size);
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};

    REQUIRE(bus.read_memory(0x04000) == 0x01);
    REQUIRE(bus.read_memory(0x08000) == 0x02);
    REQUIRE(bus.read_memory(0xEFFFF) == 0x3B);
    REQUIRE(bus.cartridge().read_switchable_bank(0, 0x0000) == 0x01);
    REQUIRE(bus.cartridge().read_switchable_bank(58, 0x3FFF) == 0x3B);
}

TEST_CASE("system SRAM maps physical 0xF0000-0xF7FFF", "[bus]") {
    vanguard8::core::Bus bus{};

    bus.write_memory(0xF0000, 0x12);
    bus.write_memory(0xF7FFF, 0x34);

    REQUIRE(bus.read_memory(0xF0000) == 0x12);
    REQUIRE(bus.read_memory(0xF7FFF) == 0x34);
}

TEST_CASE("unmapped memory and I/O return 0xFF", "[bus]") {
    vanguard8::core::Bus bus{};

    REQUIRE(bus.read_memory(0xF8000) == 0xFF);
    REQUIRE(bus.read_memory(0xFFFFF) == 0xFF);
    REQUIRE(bus.read_port(0x02) == 0xFF);
    REQUIRE(bus.read_port(0x7F) == 0xFF);
}

TEST_CASE("documented audio and video ports are routed through device surfaces", "[bus]") {
    vanguard8::core::Bus bus{};

    bus.write_port(0x40, 0x11);
    bus.write_port(0x41, 0x22);
    bus.write_port(0x50, 0x07);
    bus.write_port(0x51, 0x3E);
    bus.write_port(0x60, 0x01);
    bus.write_port(0x61, 0x0B);
    bus.write_port(0x80, 0x33);

    const auto* ym2151 = bus.port_stub(0x40);
    const auto* ym_data = bus.port_stub(0x41);
    const auto* ay_latch = bus.port_stub(0x50);
    const auto* ay_data = bus.port_stub(0x51);
    const auto* msm_control = bus.port_stub(0x60);
    const auto* msm_data = bus.port_stub(0x61);
    const auto* vdp_a_data = bus.port_stub(0x80);

    REQUIRE(ym2151 != nullptr);
    REQUIRE(ym_data != nullptr);
    REQUIRE(ay_latch != nullptr);
    REQUIRE(ay_data != nullptr);
    REQUIRE(msm_control != nullptr);
    REQUIRE(msm_data != nullptr);
    REQUIRE(vdp_a_data != nullptr);

    REQUIRE(ym2151->last_written == 0x11);
    REQUIRE(ym_data->last_written == 0x22);
    REQUIRE(ay_latch->last_written == 0x07);
    REQUIRE(ay_data->last_written == 0x3E);
    REQUIRE(msm_control->last_written == 0x01);
    REQUIRE(msm_data->last_written == 0x0B);
    REQUIRE(vdp_a_data->last_written == 0x33);
    REQUIRE(bus.read_port(0x00) == 0xFF);
    REQUIRE(bus.ym2151().latched_address() == 0x11);
    REQUIRE(bus.read_port(0x51) == 0x3E);
    REQUIRE(bus.msm5205().control() == 0x01);
    REQUIRE(bus.msm5205().latched_nibble() == 0x0B);
    REQUIRE(bus.vdp_a().vram()[0x0000] == 0x33);
    REQUIRE(bus.read_port(0x81) == 0x00);
}
