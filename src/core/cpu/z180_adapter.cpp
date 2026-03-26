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

void Z180Adapter::reset() { core_.reset(); }

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

auto Z180Adapter::in0(const std::uint8_t port) const -> std::uint8_t { return core_.in0(port); }

void Z180Adapter::out0(const std::uint8_t port, const std::uint8_t value) { core_.out0(port, value); }

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

}  // namespace vanguard8::core::cpu
