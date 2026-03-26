#include "frontend/display.hpp"

#include <fstream>
#include <stdexcept>

namespace vanguard8::frontend {

void Display::reset() { uploaded_frame_.clear(); }

void Display::upload_frame(const std::vector<std::uint8_t>& rgb_frame) {
    const auto expected_size = static_cast<std::size_t>(frame_width * frame_height * 3);
    if (rgb_frame.size() != expected_size) {
        throw std::invalid_argument("Display upload requires a full 256x212 RGB frame.");
    }
    uploaded_frame_ = rgb_frame;
}

auto Display::uploaded_frame() const -> const std::vector<std::uint8_t>& { return uploaded_frame_; }

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

    std::string ppm = "P6\n256 212\n255\n";
    ppm.append(reinterpret_cast<const char*>(uploaded_frame_.data()), static_cast<std::ptrdiff_t>(uploaded_frame_.size()));
    return ppm;
}

void Display::dump_ppm_file(const std::filesystem::path& output_path) const {
    std::ofstream stream(output_path, std::ios::binary);
    const auto ppm = dump_ppm_string();
    stream.write(ppm.data(), static_cast<std::streamsize>(ppm.size()));
}

}  // namespace vanguard8::frontend
