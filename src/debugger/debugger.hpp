#pragma once

#include "debugger/bank_panel.hpp"
#include "debugger/cpu_panel.hpp"
#include "debugger/interrupt_panel.hpp"
#include "debugger/memory_panel.hpp"
#include "debugger/vdp_panel.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace vanguard8::core {
class Emulator;
}

namespace vanguard8::frontend {
class Display;
}

namespace vanguard8::debugger {

enum class DockRegion {
    left,
    right,
    bottom,
    center,
};

enum class PanelId {
    cpu,
    memory,
    vdp,
    interrupt_log,
    bank_tracker,
};

struct PanelLayout {
    PanelId id;
    std::string_view title;
    DockRegion region = DockRegion::center;
    bool visible = true;
};

struct DockLayout {
    std::string_view ini_path;
    std::string_view dockspace_id;
    float left_ratio = 0.22F;
    float right_ratio = 0.30F;
    float bottom_ratio = 0.28F;
};

struct RenderSnapshot {
    bool rendered = false;
    bool debugger_visible = false;
    bool emulator_attached = false;
    bool display_attached = false;
    std::size_t visible_panel_count = 0;
    std::uint64_t master_cycle = 0;
    std::uint64_t cpu_tstates = 0;
    std::uint64_t completed_frames = 0;
    std::size_t event_log_size = 0;
};

class DebuggerShell {
  public:
    static constexpr std::size_t panel_count = 5;

    DebuggerShell();

    void reset();
    void attach(const core::Emulator& emulator);
    void attach(const core::Emulator& emulator, const frontend::Display& display);
    void detach();

    void set_visible(bool visible);
    void toggle_visible();

    [[nodiscard]] auto visible() const -> bool;
    [[nodiscard]] auto attached() const -> bool;
    [[nodiscard]] auto display_attached() const -> bool;
    [[nodiscard]] auto layout() const -> const DockLayout&;
    [[nodiscard]] auto panels() const -> const std::array<PanelLayout, panel_count>&;
    [[nodiscard]] auto last_render() const -> const RenderSnapshot&;
    [[nodiscard]] auto cpu_panel() -> CpuPanel&;
    [[nodiscard]] auto cpu_panel() const -> const CpuPanel&;
    [[nodiscard]] auto memory_panel() -> MemoryPanel&;
    [[nodiscard]] auto memory_panel() const -> const MemoryPanel&;
    [[nodiscard]] auto vdp_panel() -> VdpPanel&;
    [[nodiscard]] auto vdp_panel() const -> const VdpPanel&;
    [[nodiscard]] auto interrupt_panel() -> InterruptPanel&;
    [[nodiscard]] auto interrupt_panel() const -> const InterruptPanel&;
    [[nodiscard]] auto bank_panel() -> BankPanel&;
    [[nodiscard]] auto bank_panel() const -> const BankPanel&;

    [[nodiscard]] auto render() -> const RenderSnapshot&;

  private:
    const core::Emulator* emulator_ = nullptr;
    const frontend::Display* display_ = nullptr;
    bool visible_ = false;
    DockLayout layout_{
        .ini_path = "~/.config/vanguard8/imgui.ini",
        .dockspace_id = "vanguard8.debugger.root",
    };
    std::array<PanelLayout, panel_count> panels_{};
    CpuPanel cpu_panel_{};
    MemoryPanel memory_panel_{};
    VdpPanel vdp_panel_{};
    InterruptPanel interrupt_panel_{};
    BankPanel bank_panel_{};
    RenderSnapshot last_render_{};
};

}  // namespace vanguard8::debugger
