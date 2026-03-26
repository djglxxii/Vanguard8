#include <catch2/catch_test_macros.hpp>

#include "core/config.hpp"
#include "core/io/controller.hpp"
#include "core/memory/cartridge.hpp"
#include "frontend/input.hpp"
#include "frontend/rom_loader.hpp"

#include <filesystem>
#include <fstream>

TEST_CASE("controller ports are active low", "[input]") {
    vanguard8::core::io::ControllerPorts ports;

    REQUIRE(ports.read(vanguard8::core::io::Player::one) == 0xFF);
    ports.set_button(
        vanguard8::core::io::Player::one,
        vanguard8::core::io::Button::start,
        true
    );
    REQUIRE(ports.read(vanguard8::core::io::Player::one) == 0xFE);

    ports.set_button(
        vanguard8::core::io::Player::one,
        vanguard8::core::io::Button::start,
        false
    );
    REQUIRE(ports.read(vanguard8::core::io::Player::one) == 0xFF);
}

TEST_CASE("player 1 and player 2 bindings remain independent", "[input]") {
    vanguard8::core::io::ControllerPorts ports;
    vanguard8::frontend::InputManager input(ports);
    input.configure(vanguard8::core::InputBindings{});
    input.reset();

    REQUIRE(input.press_key("Return"));
    REQUIRE(ports.read(vanguard8::core::io::Player::one) == 0xFE);
    REQUIRE(ports.read(vanguard8::core::io::Player::two) == 0xFF);

    REQUIRE(input.press_key("Backspace"));
    REQUIRE(ports.read(vanguard8::core::io::Player::one) == 0xFE);
    REQUIRE(ports.read(vanguard8::core::io::Player::two) == 0xFE);

    REQUIRE(input.release_key("Return"));
    REQUIRE(ports.read(vanguard8::core::io::Player::one) == 0xFF);
    REQUIRE(ports.read(vanguard8::core::io::Player::two) == 0xFE);

    REQUIRE(input.press_gamepad_button(0, "a"));
    REQUIRE(input.press_gamepad_button(1, "a"));
    REQUIRE((ports.read(vanguard8::core::io::Player::one) & 0x08U) == 0x00U);
    REQUIRE((ports.read(vanguard8::core::io::Player::two) & 0x08U) == 0x00U);
}

TEST_CASE("ROM loading rejects oversized and invalid cartridge images", "[input]") {
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto valid_path = temp_dir / "vanguard8-valid.rom";
    const auto invalid_path = temp_dir / "vanguard8-invalid.rom";
    const auto oversized_path = temp_dir / "vanguard8-oversized.rom";

    {
        std::ofstream valid(valid_path, std::ios::binary);
        std::string bytes(0x4000, '\xAA');
        valid.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    {
        std::ofstream invalid(invalid_path, std::ios::binary);
        std::string bytes(0x2000, '\xBB');
        invalid.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    {
        std::ofstream oversized(oversized_path, std::ios::binary);
        std::string bytes(
            vanguard8::core::memory::CartridgeSlot::max_rom_size + 0x4000,
            '\xCC'
        );
        oversized.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    REQUIRE_NOTHROW(vanguard8::frontend::load_rom_file(valid_path));
    REQUIRE_THROWS_AS(vanguard8::frontend::load_rom_file(invalid_path), std::invalid_argument);
    REQUIRE_THROWS_AS(vanguard8::frontend::load_rom_file(oversized_path), std::invalid_argument);
}
