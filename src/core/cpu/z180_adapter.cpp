#include "core/cpu/z180_adapter.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace vanguard8::core::cpu {

namespace {

[[nodiscard]] auto hex_string(const std::uint32_t value, const int width) -> std::string {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << value;
    return stream.str();
}

}  // namespace

Z180Adapter::Z180Adapter(core::Bus& bus) : bus_(bus) { reset(); }

void Z180Adapter::reset() {
    pc_ = 0x0000;
    hl_ = 0x0000;
    a_ = 0x00;
    i_ = 0x00;
    il_ = 0x00;
    itc_ = 0x00;
    cbar_ = 0xFF;
    cbr_ = 0x00;
    bbr_ = 0x00;
    interrupt_mode_ = 0x00;
    iff1_ = false;
    iff2_ = false;
    halted_ = false;
}

auto Z180Adapter::pc() const -> std::uint16_t { return pc_; }

auto Z180Adapter::accumulator() const -> std::uint8_t { return a_; }

auto Z180Adapter::register_i() const -> std::uint8_t { return i_; }

auto Z180Adapter::register_il() const -> std::uint8_t { return il_; }

auto Z180Adapter::register_itc() const -> std::uint8_t { return itc_; }

auto Z180Adapter::cbar() const -> std::uint8_t { return cbar_; }

auto Z180Adapter::cbr() const -> std::uint8_t { return cbr_; }

auto Z180Adapter::bbr() const -> std::uint8_t { return bbr_; }

auto Z180Adapter::interrupt_mode() const -> std::uint8_t { return interrupt_mode_; }

auto Z180Adapter::halted() const -> bool { return halted_; }

void Z180Adapter::set_register_i(const std::uint8_t value) { i_ = value; }

void Z180Adapter::set_interrupt_mode(const std::uint8_t mode) { interrupt_mode_ = mode; }

void Z180Adapter::set_iff1(const bool enabled) { iff1_ = enabled; }

void Z180Adapter::set_iff2(const bool enabled) { iff2_ = enabled; }

auto Z180Adapter::in0(const std::uint8_t port) const -> std::uint8_t {
    switch (port) {
    case 0x33:
        return il_;
    case 0x34:
        return itc_;
    case 0x38:
        return cbr_;
    case 0x39:
        return bbr_;
    case 0x3A:
        return cbar_;
    default:
        return 0x00;
    }
}

void Z180Adapter::out0(const std::uint8_t port, const std::uint8_t value) {
    switch (port) {
    case 0x33:
        il_ = value;
        break;
    case 0x34:
        itc_ = value;
        break;
    case 0x38:
        cbr_ = value;
        break;
    case 0x39:
        bbr_ = value;
        maybe_warn_illegal_bbr();
        break;
    case 0x3A:
        cbar_ = value;
        break;
    default:
        break;
    }
}

auto Z180Adapter::translate_logical_address(const std::uint16_t logical_address) const
    -> std::uint32_t {
    if (cbar_ == 0xFF) {
        return logical_address;
    }

    const auto ca0_end_exclusive = static_cast<std::uint16_t>((cbar_ >> 4) << 12);
    const auto ca1_start = static_cast<std::uint16_t>((cbar_ & 0x0F) << 12);

    if (logical_address < ca0_end_exclusive) {
        return logical_address;
    }

    if (logical_address >= ca1_start) {
        return (static_cast<std::uint32_t>(cbr_) << 12) + (logical_address - ca1_start);
    }

    return (static_cast<std::uint32_t>(bbr_) << 12) + (logical_address - ca0_end_exclusive);
}

auto Z180Adapter::read_logical(const std::uint16_t logical_address) -> std::uint8_t {
    return bus_.read_memory(translate_logical_address(logical_address));
}

void Z180Adapter::write_logical(const std::uint16_t logical_address, const std::uint8_t value) {
    bus_.write_memory(translate_logical_address(logical_address), value);
}

auto Z180Adapter::service_pending_interrupt() -> std::optional<InterruptService> {
    if (bus_.int0_asserted() && iff1_ && interrupt_mode_ == 1) {
        return InterruptService{.source = InterruptSource::int0, .handler_address = 0x0038};
    }

    if (bus_.int1_asserted() && (itc_ & 0x02U) != 0U) {
        const auto vector_pointer = static_cast<std::uint16_t>((static_cast<std::uint16_t>(i_) << 8) | (il_ & 0xF8));
        const auto low = read_logical(vector_pointer);
        const auto high = read_logical(static_cast<std::uint16_t>(vector_pointer + 1U));
        bus_.set_int1(false);
        return InterruptService{
            .source = InterruptSource::int1,
            .handler_address = static_cast<std::uint16_t>(low | (static_cast<std::uint16_t>(high) << 8)),
        };
    }

    return std::nullopt;
}

void Z180Adapter::run_until_halt(const std::size_t max_instructions) {
    std::size_t executed = 0;
    while (!halted_) {
        if (executed >= max_instructions) {
            throw std::runtime_error("Test ROM did not halt within the instruction budget.");
        }
        execute_one();
        ++executed;
    }
}

auto Z180Adapter::fetch_byte() -> std::uint8_t {
    const auto value = read_logical(pc_);
    pc_ = static_cast<std::uint16_t>(pc_ + 1U);
    return value;
}

auto Z180Adapter::fetch_word() -> std::uint16_t {
    const auto low = fetch_byte();
    const auto high = fetch_byte();
    return static_cast<std::uint16_t>(low | (static_cast<std::uint16_t>(high) << 8));
}

void Z180Adapter::execute_one() {
    const auto opcode = fetch_byte();
    switch (opcode) {
    case 0x21:
        hl_ = fetch_word();
        return;
    case 0x22: {
        const auto address = fetch_word();
        write_logical(address, static_cast<std::uint8_t>(hl_ & 0x00FFU));
        write_logical(
            static_cast<std::uint16_t>(address + 1U),
            static_cast<std::uint8_t>((hl_ >> 8) & 0x00FFU)
        );
        return;
    }
    case 0x32: {
        const auto address = fetch_word();
        write_logical(address, a_);
        return;
    }
    case 0x3A: {
        const auto address = fetch_word();
        a_ = read_logical(address);
        return;
    }
    case 0x3E:
        a_ = fetch_byte();
        return;
    case 0x76:
        halted_ = true;
        return;
    case 0xED: {
        const auto extended_opcode = fetch_byte();
        switch (extended_opcode) {
        case 0x39: {
            const auto port = fetch_byte();
            out0(port, a_);
            return;
        }
        case 0x47:
            set_register_i(a_);
            return;
        default:
            throw std::runtime_error("Unsupported ED-prefixed opcode in test ROM.");
        }
    }
    default:
        throw std::runtime_error("Unsupported opcode in test ROM.");
    }
}

void Z180Adapter::maybe_warn_illegal_bbr() {
    if (bbr_ >= 0xF0U) {
        bus_.record_warning(
            "Illegal BBR write: " + hex_string(bbr_, 2) + " at PC=" + hex_string(pc_, 4) +
            " - maps into SRAM region"
        );
    }
}

}  // namespace vanguard8::core::cpu
