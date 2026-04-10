#pragma once

#include "frontend/display.hpp"

#include <string>
#include <vector>

namespace vanguard8::frontend {

class DisplayPresenter {
  public:
    virtual ~DisplayPresenter() = default;

    [[nodiscard]] virtual auto initialize(std::string& error) -> bool = 0;
    [[nodiscard]] virtual auto present(
        const Display& display,
        int drawable_width,
        int drawable_height,
        std::string& error
    ) -> bool = 0;
    [[nodiscard]] virtual auto readback_rgb_frame(std::string& error) const -> std::vector<std::uint8_t> = 0;
    virtual void shutdown() = 0;
};

class OpenGlDisplayPresenter final : public DisplayPresenter {
  public:
    ~OpenGlDisplayPresenter() override;

    [[nodiscard]] auto initialize(std::string& error) -> bool override;
    [[nodiscard]] auto present(
        const Display& display,
        int drawable_width,
        int drawable_height,
        std::string& error
    ) -> bool override;
    [[nodiscard]] auto readback_rgb_frame(std::string& error) const -> std::vector<std::uint8_t> override;
    void shutdown() override;

  private:
    unsigned int texture_id_ = 0;
    bool initialized_ = false;
    int texture_width_ = 0;
    int texture_height_ = 0;
};

}  // namespace vanguard8::frontend
