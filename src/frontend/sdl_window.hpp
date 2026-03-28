#pragma once

#include "frontend/runtime.hpp"

#include <array>
#include <string_view>

namespace vanguard8::frontend {

void show_frontend_error_dialog(std::string_view title, std::string_view message);

class SdlWindowHost final : public WindowHost {
  public:
    SdlWindowHost() = default;
    ~SdlWindowHost() override;

    [[nodiscard]] auto create(const WindowConfig& config, std::string& error) -> bool override;
    void drawable_size(int& width, int& height) const override;
    void pump_events(std::vector<RuntimeEvent>& events) override;
    void set_title(std::string_view title) override;
    void set_fullscreen(bool fullscreen) override;
    void present() override;
    void shutdown() override;

  private:
    void open_game_controller(int device_index);
    void close_game_controller(int instance_id);
    [[nodiscard]] auto gamepad_index_for_instance(int instance_id) const -> int;

    void* window_ = nullptr;
    void* gl_context_ = nullptr;
    bool sdl_initialized_ = false;
    std::array<void*, 2> controllers_{};
    std::array<int, 2> controller_instance_ids_{{-1, -1}};
};

}  // namespace vanguard8::frontend
