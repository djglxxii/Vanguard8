#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vanguard8::core {

struct Symbol {
    std::uint16_t address = 0;
    std::string label;
};

class SymbolTable {
  public:
    void clear();
    void load_from_file(const std::filesystem::path& path);
    void load_from_string(std::string_view contents);

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto entries() const -> const std::vector<Symbol>&;
    [[nodiscard]] auto find_exact(std::uint16_t address) const -> const Symbol*;
    [[nodiscard]] auto find_by_name(std::string_view label) const -> const Symbol*;
    [[nodiscard]] auto format_address(std::uint16_t address) const -> std::string;

  private:
    std::vector<Symbol> entries_{};
};

}  // namespace vanguard8::core
