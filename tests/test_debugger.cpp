#include <catch2/catch_test_macros.hpp>

#include "core/emulator.hpp"
#include "core/video/compositor.hpp"
#include "debugger/debugger.hpp"
#include "frontend/display.hpp"
#include "frontend/video_fixture.hpp"

namespace {

auto panel_title(
    const vanguard8::debugger::PanelLayout& panel
) -> std::string_view {
    return panel.title;
}

}  // namespace

TEST_CASE("debugger shell exposes the milestone 9 default docking layout", "[debugger]") {
    const vanguard8::debugger::DebuggerShell shell;

    REQUIRE_FALSE(shell.visible());
    REQUIRE_FALSE(shell.attached());
    REQUIRE_FALSE(shell.display_attached());
    REQUIRE(shell.layout().dockspace_id == "vanguard8.debugger.root");
    REQUIRE(shell.layout().ini_path == "~/.config/vanguard8/imgui.ini");
    REQUIRE(shell.layout().left_ratio == 0.22F);
    REQUIRE(shell.layout().right_ratio == 0.30F);
    REQUIRE(shell.layout().bottom_ratio == 0.28F);

    const auto& panels = shell.panels();
    REQUIRE(panels.size() == 5);
    REQUIRE(panels[0].id == vanguard8::debugger::PanelId::cpu);
    REQUIRE(panel_title(panels[0]) == "CPU");
    REQUIRE(panels[0].region == vanguard8::debugger::DockRegion::left);
    REQUIRE(panel_title(panels[1]) == "Memory");
    REQUIRE(panels[1].region == vanguard8::debugger::DockRegion::bottom);
    REQUIRE(panel_title(panels[2]) == "VDP");
    REQUIRE(panels[2].region == vanguard8::debugger::DockRegion::right);
    REQUIRE(panel_title(panels[3]) == "Interrupt Log");
    REQUIRE(panel_title(panels[4]) == "Bank Tracker");
    for (const auto& panel : panels) {
        REQUIRE(panel.visible);
    }
}

TEST_CASE("debugger render snapshots state without mutating emulator or display", "[debugger]") {
    vanguard8::core::Emulator emulator;
    vanguard8::frontend::build_dual_vdp_fixture_frame(
        emulator.mutable_bus().mutable_vdp_a(),
        emulator.mutable_bus().mutable_vdp_b()
    );

    const auto frame =
        vanguard8::core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
    vanguard8::frontend::Display display;
    display.upload_frame(frame);

    const auto before_master_cycle = emulator.master_cycle();
    const auto before_cpu_tstates = emulator.cpu_tstates();
    const auto before_completed_frames = emulator.completed_frames();
    const auto before_event_log_digest = emulator.event_log_digest();
    const auto before_event_log_size = emulator.event_log().size();
    const auto before_frame_digest = display.frame_digest();
    const auto before_uploaded_frame = display.uploaded_frame();

    vanguard8::debugger::DebuggerShell shell;
    shell.attach(emulator, display);
    shell.set_visible(true);

    const auto& snapshot = shell.render();

    REQUIRE(snapshot.rendered);
    REQUIRE(snapshot.debugger_visible);
    REQUIRE(snapshot.emulator_attached);
    REQUIRE(snapshot.display_attached);
    REQUIRE(snapshot.visible_panel_count == 5);
    REQUIRE(snapshot.master_cycle == before_master_cycle);
    REQUIRE(snapshot.cpu_tstates == before_cpu_tstates);
    REQUIRE(snapshot.completed_frames == before_completed_frames);
    REQUIRE(snapshot.event_log_size == before_event_log_size);

    REQUIRE(emulator.master_cycle() == before_master_cycle);
    REQUIRE(emulator.cpu_tstates() == before_cpu_tstates);
    REQUIRE(emulator.completed_frames() == before_completed_frames);
    REQUIRE(emulator.event_log_digest() == before_event_log_digest);
    REQUIRE(emulator.event_log().size() == before_event_log_size);
    REQUIRE(display.frame_digest() == before_frame_digest);
    REQUIRE(display.uploaded_frame() == before_uploaded_frame);
}
