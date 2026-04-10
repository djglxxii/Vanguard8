#include "frontend/display.hpp"

#include <fstream>
#include <stdexcept>

namespace vanguard8::frontend {

void Display::reset() {
    uploaded_frame_.clear();
    uploaded_frame_width_ = 0;
}

void Display::upload_frame(const std::vector<std::uint8_t>& rgb_frame) {
    const auto bytes_per_row = static_cast<std::size_t>(frame_height * 3);
    if (rgb_frame.empty() || (rgb_frame.size() % bytes_per_row) != 0U) {
        throw std::invalid_argument("Display upload requires a full RGB frame aligned to 212 lines.");
    }

    const auto inferred_width = static_cast<int>(rgb_frame.size() / bytes_per_row);
    if (inferred_width != core::video::V9938::visible_width &&
        inferred_width != core::video::V9938::max_visible_width) {
        throw std::invalid_argument("Display upload only supports 256x212 or 512x212 RGB frames.");
    }
    uploaded_frame_ = rgb_frame;
    uploaded_frame_width_ = inferred_width;
}

auto Display::uploaded_frame() const -> const std::vector<std::uint8_t>& { return uploaded_frame_; }

auto Display::frame_width() const -> int { return uploaded_frame_width_; }

auto Display::uploaded_frame_height() const -> int { return frame_height; }

auto Display::frame_digest() const -> std::uint64_t {
    std::uint64_t digest = 1469598103934665603ULL;
    for (const auto byte : uploaded_frame_) {
        digest ^= byte;
        digest *= 1099511628211ULL;
    }
    return digest;
}

auto Display::dump_ppm_string() const -> std::string {
    if (uploaded_frame_.empty()) {
        return {};
    }

    std::string ppm =
        "P6\n" + std::to_string(frame_width()) + " " + std::to_string(uploaded_frame_height()) + "\n255\n";
    ppm.append(reinterpret_cast<const char*>(uploaded_frame_.data()), static_cast<std::ptrdiff_t>(uploaded_frame_.size()));
    return ppm;
}

void Display::dump_ppm_file(const std::filesystem::path& output_path) const {
    std::ofstream stream(output_path, std::ios::binary);
    const auto ppm = dump_ppm_string();
    stream.write(ppm.data(), static_cast<std::streamsize>(ppm.size()));
}

}  // namespace vanguard8::frontend
