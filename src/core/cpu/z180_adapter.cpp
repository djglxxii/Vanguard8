#include "core/cpu/z180_adapter.hpp"

#include <sstream>

namespace vanguard8::core::cpu {

namespace {

constexpr std::uint8_t flag_zero = 0x40;

}

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
          .observe_logical_memory_read =
              [this](const std::uint16_t address, const std::uint8_t value) {
                  record_breakpoint_hit(BreakpointType::memory_read, address, value);
              },
          .observe_logical_memory_write =
              [this](const std::uint16_t address, const std::uint8_t value) {
                  record_breakpoint_hit(BreakpointType::memory_write, address, value);
              },
          .observe_internal_io_read =
              [this](const std::uint16_t port, const std::uint8_t value) {
                  record_breakpoint_hit(BreakpointType::io_read, port, value);
              },
          .observe_internal_io_write =
              [this](const std::uint16_t port, const std::uint8_t value) {
                  record_breakpoint_hit(BreakpointType::io_write, port, value);
                  if (port == 0x39) {
                      record_bank_switch(value);
                  }
              },
          .record_warning = [this](std::string message) { bus_.record_warning(std::move(message)); },
          .acknowledge_int1 = [this]() { bus_.set_int1(false); },
      }) {
    reset();
}

void Z180Adapter::reset() {
    core_.reset();
    dma_ = {};
    breakpoint_hits_.clear();
    bank_switch_log_.clear();
    pending_breakpoint_hit_.reset();
    bank_switch_sequence_ = 0;
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

auto Z180Adapter::state_snapshot() const -> CpuStateSnapshot {
    return CpuStateSnapshot{
        .registers = core_.register_snapshot(),
        .dma0 =
            DmaChannel0Snapshot{
                .source = dma_channel0_source(),
                .destination = dma_channel0_destination(),
                .length = dma_channel0_length(),
            },
        .dma1 =
            DmaChannel1Snapshot{
                .memory_address = dma_channel1_memory_address(),
                .port = dma_channel1_port(),
                .length = dma_channel1_length(),
            },
        .dstat = dma_.dstat,
        .dmode = dma_.dmode,
        .dcntl = dma_.dcntl,
    };
}

void Z180Adapter::load_state_snapshot(const CpuStateSnapshot& state) {
    core_.load_register_snapshot(state.registers);
    dma_.sar0l = static_cast<std::uint8_t>(state.dma0.source & 0xFFU);
    dma_.sar0h = static_cast<std::uint8_t>((state.dma0.source >> 8U) & 0xFFU);
    dma_.sar0b = static_cast<std::uint8_t>((state.dma0.source >> 16U) & 0x0FU);
    dma_.dar0l = static_cast<std::uint8_t>(state.dma0.destination & 0xFFU);
    dma_.dar0h = static_cast<std::uint8_t>((state.dma0.destination >> 8U) & 0xFFU);
    dma_.dar0b = static_cast<std::uint8_t>((state.dma0.destination >> 16U) & 0x0FU);
    dma_.bcr0l = static_cast<std::uint8_t>(state.dma0.length & 0xFFU);
    dma_.bcr0h = static_cast<std::uint8_t>((state.dma0.length >> 8U) & 0xFFU);
    dma_.mar1l = static_cast<std::uint8_t>(state.dma1.memory_address & 0xFFU);
    dma_.mar1h = static_cast<std::uint8_t>((state.dma1.memory_address >> 8U) & 0xFFU);
    dma_.mar1b = static_cast<std::uint8_t>((state.dma1.memory_address >> 16U) & 0x0FU);
    dma_.iar1l = static_cast<std::uint8_t>(state.dma1.port & 0xFFU);
    dma_.iar1h = static_cast<std::uint8_t>((state.dma1.port >> 8U) & 0xFFU);
    dma_.bcr1l = static_cast<std::uint8_t>(state.dma1.length & 0xFFU);
    dma_.bcr1h = static_cast<std::uint8_t>((state.dma1.length >> 8U) & 0xFFU);
    dma_.dstat = state.dstat;
    dma_.dmode = state.dmode;
    dma_.dcntl = state.dcntl;
}

auto Z180Adapter::breakpoints() const -> const std::vector<Breakpoint>& { return breakpoints_; }

auto Z180Adapter::breakpoint_hits() const -> const std::vector<BreakpointHit>& { return breakpoint_hits_; }

auto Z180Adapter::bank_switch_log() const -> const std::vector<BankSwitchEvent>& {
    return bank_switch_log_;
}

void Z180Adapter::set_register_i(const std::uint8_t value) { core_.set_register_i(value); }

void Z180Adapter::set_interrupt_mode(const std::uint8_t mode) { core_.set_interrupt_mode(mode); }

void Z180Adapter::set_iff1(const bool enabled) { core_.set_iff1(enabled); }

void Z180Adapter::set_iff2(const bool enabled) { core_.set_iff2(enabled); }

auto Z180Adapter::add_breakpoint(Breakpoint breakpoint) -> std::size_t {
    breakpoint.id = next_breakpoint_id_++;
    breakpoints_.push_back(breakpoint);
    return breakpoint.id;
}

void Z180Adapter::clear_breakpoints() { breakpoints_.clear(); }

void Z180Adapter::clear_breakpoint_hits() {
    breakpoint_hits_.clear();
    pending_breakpoint_hit_.reset();
}

void Z180Adapter::clear_bank_switch_log() { bank_switch_log_.clear(); }

auto Z180Adapter::in0(const std::uint8_t port) -> std::uint8_t {
    if (const auto value = read_dma_register(port); value.has_value()) {
        record_breakpoint_hit(BreakpointType::io_read, port, *value);
        return *value;
    }

    return core_.in0(port);
}

void Z180Adapter::out0(const std::uint8_t port, const std::uint8_t value) {
    if (write_dma_register(port, value)) {
        record_breakpoint_hit(BreakpointType::io_write, port, value);
        return;
    }

    core_.out0(port, value);
}

void Z180Adapter::advance_tstates(const std::uint64_t tstates) { core_.advance_tstates(tstates); }

auto Z180Adapter::next_scheduled_tstates() const -> std::uint64_t {
    if (const auto interrupt = next_interrupt_service_source(); interrupt.has_value()) {
        return interrupt_service_tstates(*interrupt);
    }

    if (halted()) {
        return 0;
    }

    return current_instruction_tstates();
}

auto Z180Adapter::step_scheduled_instruction() -> std::uint64_t {
    if (const auto service = service_pending_interrupt(); service.has_value()) {
        return interrupt_service_tstates(service->source);
    }

    if (halted()) {
        return 0;
    }

    const auto tstates = current_instruction_tstates();
    step_instruction();
    return tstates;
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

auto Z180Adapter::peek_logical(const std::uint16_t logical_address) const -> std::uint8_t {
    const auto physical_address = translate_logical_address(logical_address);
    if (memory::CartridgeSlot::contains_physical_address(physical_address)) {
        return bus_.cartridge().read_physical(physical_address);
    }

    if (memory::Sram::contains_physical_address(physical_address)) {
        return bus_.sram().read(physical_address);
    }

    return core::Bus::open_bus_value;
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
        .source = service->source == third_party::z180::InterruptSource::int0
                      ? InterruptSource::int0
                  : service->source == third_party::z180::InterruptSource::int1
                      ? InterruptSource::int1
                  : service->source == third_party::z180::InterruptSource::prt0
                      ? InterruptSource::prt0
                      : InterruptSource::prt1,
        .handler_address = service->handler_address,
    };
}

void Z180Adapter::step_instruction() { core_.step_one(); }

void Z180Adapter::run_until_halt(const std::size_t max_instructions) { core_.run_until_halt(max_instructions); }

auto Z180Adapter::run_until_breakpoint_or_halt(const std::size_t max_instructions)
    -> BreakpointRunResult {
    BreakpointRunResult result;
    pending_breakpoint_hit_.reset();

    while (!halted()) {
        if (result.executed_instructions >= max_instructions) {
            throw std::runtime_error("Test ROM did not halt or hit a breakpoint within the instruction budget.");
        }

        record_breakpoint_hit(BreakpointType::pc, core_.pc());
        if (pending_breakpoint_hit_.has_value()) {
            result.breakpoint_hit = true;
            result.hit = pending_breakpoint_hit_;
            break;
        }

        core_.step_one();
        ++result.executed_instructions;
        if (pending_breakpoint_hit_.has_value()) {
            result.breakpoint_hit = true;
            result.hit = pending_breakpoint_hit_;
            break;
        }
    }

    result.halted = halted();
    pending_breakpoint_hit_.reset();
    return result;
}

auto Z180Adapter::next_interrupt_service_source() const -> std::optional<InterruptSource> {
    const auto registers = state_snapshot().registers;

    if (bus_.int0_asserted() && registers.iff1 && registers.interrupt_mode == 1) {
        return InterruptSource::int0;
    }
    if (bus_.int1_asserted() && registers.iff1 && (registers.itc & 0x02U) != 0U) {
        return InterruptSource::int1;
    }
    if (registers.iff1 && (registers.tcr & 0x50U) == 0x50U) {
        return InterruptSource::prt0;
    }
    if (registers.iff1 && (registers.tcr & 0xA0U) == 0xA0U) {
        return InterruptSource::prt1;
    }
    return std::nullopt;
}

auto Z180Adapter::interrupt_service_tstates(const InterruptSource source) const -> std::uint64_t {
    switch (source) {
    case InterruptSource::int0:
        return 13;
    case InterruptSource::int1:
    case InterruptSource::prt0:
    case InterruptSource::prt1:
        return 13;
    }
    return 13;
}

auto Z180Adapter::current_instruction_tstates() const -> std::uint64_t {
    const auto opcode = peek_logical(pc());

    switch (opcode) {
    case 0x00:
    case 0x0F:
    case 0xAF:
    case 0xF3:
    case 0xFB:
        return 4;
    case 0x18:
        return 12;
    case 0x20:
        return (core_.register_snapshot().af & flag_zero) == 0U ? 12 : 7;
    case 0x21:
    case 0x2A:
    case 0x31:
    case 0xC3:
        return 10;
    case 0x23:
        return 6;
    case 0x22:
        return 16;
    case 0x32:
    case 0x3A:
        return 13;
    case 0x3E:
    case 0x06:
    case 0x7E:
    case 0xE6:
        return 7;
    case 0x76:
        return 4;
    case 0xC9:
    case 0xF1:
        return 10;
    case 0xCD:
        return 17;
    case 0xD3:
    case 0xDB:
        return 11;
    case 0xED:
        return ed_instruction_tstates(peek_logical(static_cast<std::uint16_t>(pc() + 1U)));
    case 0xF5:
        return 11;
    default:
        {
            std::ostringstream stream;
            stream << "Unsupported timed Z180 opcode 0x" << std::uppercase << std::hex
                   << static_cast<int>(opcode) << " at PC 0x" << pc();
            throw std::runtime_error(stream.str());
        }
    }
}

auto Z180Adapter::ed_instruction_tstates(const std::uint8_t opcode) const -> std::uint64_t {
    switch (opcode) {
    case 0x39:
        return 12;
    case 0x47:
        return 9;
    case 0x4D:
        return 14;
    case 0x56:
        return 8;
    case 0x64:
        return 9;
    default:
        if ((opcode & 0xC7U) == 0x00U && ((opcode >> 3U) & 0x07U) != 0x06U) {
            return 12;
        }
        if ((opcode & 0xC7U) == 0x01U && ((opcode >> 3U) & 0x07U) != 0x06U) {
            return 12;
        }
        if ((opcode & 0xC7U) == 0x04U && ((opcode >> 3U) & 0x07U) != 0x06U) {
            return 8;
        }
        if ((opcode & 0xCFU) == 0x4CU) {
            return 17;
        }
        {
            std::ostringstream stream;
            stream << "Unsupported timed Z180 ED opcode 0x" << std::uppercase << std::hex
                   << static_cast<int>(opcode) << " at PC 0x" << pc();
            throw std::runtime_error(stream.str());
        }
    }
}

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

void Z180Adapter::record_breakpoint_hit(
    const BreakpointType type,
    const std::uint16_t address,
    const std::optional<std::uint8_t> value
) {
    if (pending_breakpoint_hit_.has_value()) {
        return;
    }

    for (const auto& breakpoint : breakpoints_) {
        if (!breakpoint.enabled || breakpoint.type != type || breakpoint.address != address) {
            continue;
        }
        if (breakpoint.value.has_value() && breakpoint.value != value) {
            continue;
        }

        pending_breakpoint_hit_ = BreakpointHit{
            .breakpoint = breakpoint,
            .pc = core_.pc(),
            .address = address,
            .value = value,
        };
        breakpoint_hits_.push_back(*pending_breakpoint_hit_);
        constexpr std::size_t max_breakpoint_hits = 256;
        if (breakpoint_hits_.size() > max_breakpoint_hits) {
            breakpoint_hits_.erase(breakpoint_hits_.begin());
        }
        return;
    }
}

void Z180Adapter::record_bank_switch(const std::uint8_t value) {
    bank_switch_log_.push_back(BankSwitchEvent{
        .sequence = bank_switch_sequence_++,
        .pc = core_.pc(),
        .bbr = value,
        .bank = value >= 0x04U ? static_cast<int>(value / 4U) - 1 : -1,
        .legal = value < 0xF0U,
    });
    constexpr std::size_t max_bank_switch_entries = 256;
    if (bank_switch_log_.size() > max_bank_switch_entries) {
        bank_switch_log_.erase(bank_switch_log_.begin());
    }
}

}  // namespace vanguard8::core::cpu
