#pragma once

#include "core/video/v9938.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace vanguard8::frontend {

class Display {
  public:
    static constexpr int default_frame_width = vanguard8::core::video::V9938::visible_width;
    static constexpr int frame_height = vanguard8::core::video::V9938::visible_height;

    void reset();
    void upload_frame(const std::vector<std::uint8_t>& rgb_frame);
    [[nodiscard]] auto uploaded_frame() const -> const std::vector<std::uint8_t>&;
    [[nodiscard]] auto frame_width() const -> int;
    [[nodiscard]] auto uploaded_frame_height() const -> int;
    [[nodiscard]] auto frame_digest() const -> std::uint64_t;
    [[nodiscard]] auto dump_ppm_string() const -> std::string;
    void dump_ppm_file(const std::filesystem::path& output_path) const;

  private:
    std::vector<std::uint8_t> uploaded_frame_;
    int uploaded_frame_width_ = 0;
};

}  // namespace vanguard8::frontend
