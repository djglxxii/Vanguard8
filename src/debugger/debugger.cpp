#include "debugger/debugger.hpp"

#include "core/emulator.hpp"

namespace vanguard8::debugger {

DebuggerShell::DebuggerShell()
    : panels_({{
          PanelLayout{.id = PanelId::cpu, .title = "CPU", .region = DockRegion::left, .visible = true},
          PanelLayout{.id = PanelId::memory, .title = "Memory", .region = DockRegion::bottom, .visible = true},
          PanelLayout{.id = PanelId::vdp, .title = "VDP", .region = DockRegion::right, .visible = true},
          PanelLayout{
              .id = PanelId::interrupt_log,
              .title = "Interrupt Log",
              .region = DockRegion::bottom,
              .visible = true,
          },
          PanelLayout{
              .id = PanelId::bank_tracker,
              .title = "Bank Tracker",
              .region = DockRegion::right,
              .visible = true,
          },
      }}) {}

void DebuggerShell::reset() {
    emulator_ = nullptr;
    display_ = nullptr;
    visible_ = false;
    last_render_ = {};
}

void DebuggerShell::attach(const core::Emulator& emulator) {
    emulator_ = &emulator;
    display_ = nullptr;
}

void DebuggerShell::attach(const core::Emulator& emulator, const frontend::Display& display) {
    emulator_ = &emulator;
    display_ = &display;
}

void DebuggerShell::detach() {
    emulator_ = nullptr;
    display_ = nullptr;
    last_render_ = {};
}

void DebuggerShell::set_visible(const bool visible) { visible_ = visible; }

void DebuggerShell::toggle_visible() { visible_ = !visible_; }

auto DebuggerShell::visible() const -> bool { return visible_; }

auto DebuggerShell::attached() const -> bool { return emulator_ != nullptr; }

auto DebuggerShell::display_attached() const -> bool { return display_ != nullptr; }

auto DebuggerShell::layout() const -> const DockLayout& { return layout_; }

auto DebuggerShell::panels() const -> const std::array<PanelLayout, panel_count>& { return panels_; }

auto DebuggerShell::last_render() const -> const RenderSnapshot& { return last_render_; }

auto DebuggerShell::cpu_panel() -> CpuPanel& { return cpu_panel_; }

auto DebuggerShell::cpu_panel() const -> const CpuPanel& { return cpu_panel_; }

auto DebuggerShell::memory_panel() -> MemoryPanel& { return memory_panel_; }

auto DebuggerShell::memory_panel() const -> const MemoryPanel& { return memory_panel_; }

auto DebuggerShell::vdp_panel() -> VdpPanel& { return vdp_panel_; }

auto DebuggerShell::vdp_panel() const -> const VdpPanel& { return vdp_panel_; }

auto DebuggerShell::render() -> const RenderSnapshot& {
    last_render_ = RenderSnapshot{
        .rendered = false,
        .debugger_visible = visible_,
        .emulator_attached = emulator_ != nullptr,
        .display_attached = display_ != nullptr,
    };

    if (!visible_ || emulator_ == nullptr) {
        return last_render_;
    }

    std::size_t visible_panel_count = 0;
    for (const auto& panel : panels_) {
        if (panel.visible) {
            ++visible_panel_count;
        }
    }

    last_render_ = RenderSnapshot{
        .rendered = true,
        .debugger_visible = visible_,
        .emulator_attached = true,
        .display_attached = display_ != nullptr,
        .visible_panel_count = visible_panel_count,
        .master_cycle = emulator_->master_cycle(),
        .cpu_tstates = emulator_->cpu_tstates(),
        .completed_frames = emulator_->completed_frames(),
        .event_log_size = emulator_->event_log().size(),
    };
    return last_render_;
}

}  // namespace vanguard8::debugger
