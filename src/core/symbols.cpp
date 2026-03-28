#include "core/symbols.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace vanguard8::core {

namespace {

auto trim(const std::string_view input) -> std::string_view {
    auto start = std::size_t{0};
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start])) != 0) {
        ++start;
    }

    auto end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1U])) != 0) {
        --end;
    }

    return input.substr(start, end - start);
}

auto is_valid_label(const std::string_view label) -> bool {
    if (label.empty()) {
        return false;
    }

    for (const auto ch : label) {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_') {
            return false;
        }
    }
    return true;
}

auto hex16(const std::uint16_t value) -> std::string {
    std::ostringstream stream;
    stream << std::uppercase << std::hex;
    stream.width(4);
    stream.fill('0');
    stream << value;
    return stream.str();
}

}  // namespace

void SymbolTable::clear() { entries_.clear(); }

void SymbolTable::load_from_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open symbol file.");
    }

    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    load_from_string(contents);
}

void SymbolTable::load_from_string(const std::string_view contents) {
    clear();

    std::istringstream stream{std::string(contents)};
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(stream, line)) {
        ++line_number;
        const auto trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == ';') {
            continue;
        }

        std::istringstream line_stream{std::string(trimmed)};
        std::string address_token;
        std::string label_token;
        line_stream >> address_token >> label_token;
        if (address_token.empty() || label_token.empty()) {
            throw std::invalid_argument("Invalid symbol line " + std::to_string(line_number) + ".");
        }

        if (address_token.size() > 4U || address_token.find("0x") != std::string::npos ||
            address_token.find("0X") != std::string::npos) {
            throw std::invalid_argument("Invalid symbol address on line " + std::to_string(line_number) + ".");
        }

        if (!is_valid_label(label_token)) {
            throw std::invalid_argument("Invalid symbol label on line " + std::to_string(line_number) + ".");
        }

        const auto address = static_cast<std::uint16_t>(std::stoul(address_token, nullptr, 16));
        if (find_exact(address) != nullptr) {
            throw std::invalid_argument(
                "Duplicate symbol address " + hex16(address) + " on line " + std::to_string(line_number) + "."
            );
        }
        if (find_by_name(label_token) != nullptr) {
            throw std::invalid_argument(
                "Duplicate symbol label '" + label_token + "' on line " + std::to_string(line_number) + "."
            );
        }

        entries_.push_back(Symbol{
            .address = address,
            .label = label_token,
        });
    }

    std::sort(entries_.begin(), entries_.end(), [](const Symbol& lhs, const Symbol& rhs) {
        return lhs.address < rhs.address;
    });
}

auto SymbolTable::size() const -> std::size_t { return entries_.size(); }

auto SymbolTable::empty() const -> bool { return entries_.empty(); }

auto SymbolTable::entries() const -> const std::vector<Symbol>& { return entries_; }

auto SymbolTable::find_exact(const std::uint16_t address) const -> const Symbol* {
    const auto it = std::find_if(entries_.begin(), entries_.end(), [address](const Symbol& entry) {
        return entry.address == address;
    });
    return it == entries_.end() ? nullptr : &*it;
}

auto SymbolTable::find_by_name(const std::string_view label) const -> const Symbol* {
    const auto it = std::find_if(entries_.begin(), entries_.end(), [label](const Symbol& entry) {
        return entry.label == label;
    });
    return it == entries_.end() ? nullptr : &*it;
}

auto SymbolTable::format_address(const std::uint16_t address) const -> std::string {
    if (const auto* exact = find_exact(address); exact != nullptr) {
        return exact->label;
    }

    const auto it = std::upper_bound(entries_.begin(), entries_.end(), address, [](const std::uint16_t value, const Symbol& entry) {
        return value < entry.address;
    });
    if (it == entries_.begin()) {
        return {};
    }

    const auto& nearest = *std::prev(it);
    if (nearest.address > address) {
        return {};
    }

    std::ostringstream stream;
    stream << nearest.label << "+0x" << std::uppercase << std::hex << (address - nearest.address);
    return stream.str();
}

}  // namespace vanguard8::core
