#include "core/cpu/z180_adapter.hpp"

namespace vanguard8::core::cpu {

Z180Adapter::Z180Adapter(core::Bus& bus)
    : bus_(bus),
      core_(third_party::z180::Callbacks{
          .read_memory = [this](const std::uint32_t address) { return bus_.read_memory(address); },
          .write_memory = [this](const std::uint32_t address, const std::uint8_t value) {
              bus_.write_memory(address, value);
          },
          .read_port = [this](const std::uint16_t port) { return bus_.read_port(port); },
          .write_port = [this](const std::uint16_t port, const std::uint8_t value) {
              bus_.write_port(port, value);
          },
          .record_warning = [this](std::string message) { bus_.record_warning(std::move(message)); },
          .acknowledge_int1 = [this]() { bus_.set_int1(false); },
      }) {
    reset();
}

void Z180Adapter::reset() {
    core_.reset();
    dma_ = {};
}

auto Z180Adapter::pc() const -> std::uint16_t { return core_.pc(); }

auto Z180Adapter::accumulator() const -> std::uint8_t { return core_.accumulator(); }

auto Z180Adapter::register_i() const -> std::uint8_t { return core_.register_i(); }

auto Z180Adapter::register_il() const -> std::uint8_t { return core_.register_il(); }

auto Z180Adapter::register_itc() const -> std::uint8_t { return core_.register_itc(); }

auto Z180Adapter::cbar() const -> std::uint8_t { return core_.cbar(); }

auto Z180Adapter::cbr() const -> std::uint8_t { return core_.cbr(); }

auto Z180Adapter::bbr() const -> std::uint8_t { return core_.bbr(); }

auto Z180Adapter::interrupt_mode() const -> std::uint8_t { return core_.interrupt_mode(); }

auto Z180Adapter::halted() const -> bool { return core_.halted(); }

void Z180Adapter::set_register_i(const std::uint8_t value) { core_.set_register_i(value); }

void Z180Adapter::set_interrupt_mode(const std::uint8_t mode) { core_.set_interrupt_mode(mode); }

void Z180Adapter::set_iff1(const bool enabled) { core_.set_iff1(enabled); }

void Z180Adapter::set_iff2(const bool enabled) { core_.set_iff2(enabled); }

auto Z180Adapter::in0(const std::uint8_t port) const -> std::uint8_t {
    if (const auto value = read_dma_register(port); value.has_value()) {
        return *value;
    }

    return core_.in0(port);
}

void Z180Adapter::out0(const std::uint8_t port, const std::uint8_t value) {
    if (write_dma_register(port, value)) {
        return;
    }

    core_.out0(port, value);
}

void Z180Adapter::execute_dma(const DmaChannel channel) {
    if (!dma_mode_supported(channel)) {
        return;
    }

    switch (channel) {
    case DmaChannel::channel0: {
        const auto source = dma_channel0_source();
        const auto destination = dma_channel0_destination();
        const auto length = dma_channel0_length();
        for (std::uint32_t offset = 0; offset < length; ++offset) {
            bus_.write_memory(destination + offset, bus_.read_memory(source + offset));
        }
        return;
    }
    case DmaChannel::channel1: {
        const auto source = dma_channel1_memory_address();
        const auto port = dma_channel1_port();
        const auto length = dma_channel1_length();
        for (std::uint32_t offset = 0; offset < length; ++offset) {
            bus_.write_port(port, bus_.read_memory(source + offset));
        }
        return;
    }
    }
}

auto Z180Adapter::translate_logical_address(const std::uint16_t logical_address) const
    -> std::uint32_t {
    return core_.translate_logical_address(logical_address);
}

auto Z180Adapter::read_logical(const std::uint16_t logical_address) -> std::uint8_t {
    return core_.read_logical(logical_address);
}

void Z180Adapter::write_logical(const std::uint16_t logical_address, const std::uint8_t value) {
    core_.write_logical(logical_address, value);
}

auto Z180Adapter::service_pending_interrupt() -> std::optional<InterruptService> {
    const auto service = core_.service_pending_interrupt(bus_.int0_asserted(), bus_.int1_asserted());
    if (!service.has_value()) {
        return std::nullopt;
    }

    return InterruptService{
        .source = service->source == third_party::z180::InterruptSource::int0 ? InterruptSource::int0
                                                                              : InterruptSource::int1,
        .handler_address = service->handler_address,
    };
}

void Z180Adapter::run_until_halt(const std::size_t max_instructions) { core_.run_until_halt(max_instructions); }

auto Z180Adapter::read_dma_register(const std::uint8_t port) const -> std::optional<std::uint8_t> {
    switch (port) {
    case 0x20:
        return dma_.sar0l;
    case 0x21:
        return dma_.sar0h;
    case 0x22:
        return dma_.sar0b;
    case 0x23:
        return dma_.dar0l;
    case 0x24:
        return dma_.dar0h;
    case 0x25:
        return dma_.dar0b;
    case 0x26:
        return dma_.bcr0l;
    case 0x27:
        return dma_.bcr0h;
    case 0x28:
        return dma_.mar1l;
    case 0x29:
        return dma_.mar1h;
    case 0x2A:
        return dma_.mar1b;
    case 0x2B:
        return dma_.iar1l;
    case 0x2C:
        return dma_.iar1h;
    case 0x2D:
        return 0x00;
    case 0x2E:
        return dma_.bcr1l;
    case 0x2F:
        return dma_.bcr1h;
    case 0x30:
        return dma_.dstat;
    case 0x31:
        return dma_.dmode;
    case 0x32:
        return dma_.dcntl;
    default:
        return std::nullopt;
    }
}

auto Z180Adapter::write_dma_register(const std::uint8_t port, const std::uint8_t value) -> bool {
    switch (port) {
    case 0x20:
        dma_.sar0l = value;
        return true;
    case 0x21:
        dma_.sar0h = value;
        return true;
    case 0x22:
        dma_.sar0b = value;
        return true;
    case 0x23:
        dma_.dar0l = value;
        return true;
    case 0x24:
        dma_.dar0h = value;
        return true;
    case 0x25:
        dma_.dar0b = value;
        return true;
    case 0x26:
        dma_.bcr0l = value;
        return true;
    case 0x27:
        dma_.bcr0h = value;
        return true;
    case 0x28:
        dma_.mar1l = value;
        return true;
    case 0x29:
        dma_.mar1h = value;
        return true;
    case 0x2A:
        dma_.mar1b = value;
        return true;
    case 0x2B:
        dma_.iar1l = value;
        return true;
    case 0x2C:
        dma_.iar1h = value;
        return true;
    case 0x2D:
        return true;
    case 0x2E:
        dma_.bcr1l = value;
        return true;
    case 0x2F:
        dma_.bcr1h = value;
        return true;
    case 0x30:
        dma_.dstat = value;
        return true;
    case 0x31:
        dma_.dmode = value;
        return true;
    case 0x32:
        dma_.dcntl = value;
        return true;
    default:
        return false;
    }
}

auto Z180Adapter::dma_channel0_source() const -> std::uint32_t {
    return static_cast<std::uint32_t>(dma_.sar0l) |
           (static_cast<std::uint32_t>(dma_.sar0h) << 8U) |
           ((static_cast<std::uint32_t>(dma_.sar0b) & 0x0FU) << 16U);
}

auto Z180Adapter::dma_channel0_destination() const -> std::uint32_t {
    return static_cast<std::uint32_t>(dma_.dar0l) |
           (static_cast<std::uint32_t>(dma_.dar0h) << 8U) |
           ((static_cast<std::uint32_t>(dma_.dar0b) & 0x0FU) << 16U);
}

auto Z180Adapter::dma_channel0_length() const -> std::uint16_t {
    return static_cast<std::uint16_t>(
        dma_.bcr0l | (static_cast<std::uint16_t>(dma_.bcr0h) << 8U)
    );
}

auto Z180Adapter::dma_channel1_memory_address() const -> std::uint32_t {
    return static_cast<std::uint32_t>(dma_.mar1l) |
           (static_cast<std::uint32_t>(dma_.mar1h) << 8U) |
           ((static_cast<std::uint32_t>(dma_.mar1b) & 0x0FU) << 16U);
}

auto Z180Adapter::dma_channel1_port() const -> std::uint16_t {
    return static_cast<std::uint16_t>(
        dma_.iar1l | (static_cast<std::uint16_t>(dma_.iar1h & 0x01U) << 8U)
    );
}

auto Z180Adapter::dma_channel1_length() const -> std::uint16_t {
    return static_cast<std::uint16_t>(
        dma_.bcr1l | (static_cast<std::uint16_t>(dma_.bcr1h) << 8U)
    );
}

auto Z180Adapter::dma_mode_supported(const DmaChannel channel) -> bool {
    // The repo spec documents channel roles and routing, but not the DMODE/DCNTL bit-level
    // matrix. Milestone 8 starts with the documented sequential transfer paths only.
    if (dma_.dmode != 0x00U || dma_.dcntl != 0x00U) {
        bus_.record_warning("DMA execution skipped: unsupported DMODE/DCNTL configuration.");
        return false;
    }

    if (channel == DmaChannel::channel0 && dma_channel0_length() == 0U) {
        return false;
    }

    if (channel == DmaChannel::channel1 && dma_channel1_length() == 0U) {
        return false;
    }

    return true;
}

}  // namespace vanguard8::core::cpu
