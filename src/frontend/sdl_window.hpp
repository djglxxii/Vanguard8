#pragma once

#include "frontend/runtime.hpp"

namespace vanguard8::frontend {

class SdlWindowHost final : public WindowHost {
  public:
    SdlWindowHost() = default;
    ~SdlWindowHost() override;

    [[nodiscard]] auto create(const WindowConfig& config, std::string& error) -> bool override;
    void pump_events(std::vector<RuntimeEvent>& events) override;
    void present() override;
    void shutdown() override;

  private:
    void* window_ = nullptr;
    void* gl_context_ = nullptr;
    bool sdl_initialized_ = false;
};

}  // namespace vanguard8::frontend
