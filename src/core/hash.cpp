#include "core/hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vanguard8::core {

namespace {

constexpr std::array<std::uint32_t, 64> k_table = {
    0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U, 0x3956C25BU, 0x59F111F1U, 0x923F82A4U,
    0xAB1C5ED5U, 0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U, 0x72BE5D74U, 0x80DEB1FEU,
    0x9BDC06A7U, 0xC19BF174U, 0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU, 0x2DE92C6FU,
    0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU, 0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
    0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U, 0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU,
    0x53380D13U, 0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U, 0xA2BFE8A1U, 0xA81A664BU,
    0xC24B8B70U, 0xC76C51A3U, 0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U, 0x19A4C116U,
    0x1E376C08U, 0x2748774CU, 0x34B0BCB5U, 0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
    0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U, 0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U,
    0xC67178F2U,
};

constexpr auto rotr(const std::uint32_t value, const int shift) -> std::uint32_t {
    return (value >> shift) | (value << (32 - shift));
}

struct Context {
    std::array<std::uint32_t, 8> state = {
        0x6A09E667U,
        0xBB67AE85U,
        0x3C6EF372U,
        0xA54FF53AU,
        0x510E527FU,
        0x9B05688CU,
        0x1F83D9ABU,
        0x5BE0CD19U,
    };
    std::uint64_t total_bytes = 0;
    std::array<std::uint8_t, 64> buffer{};
    std::size_t buffered = 0;
};

void process_block(Context& context, const std::uint8_t* block) {
    std::array<std::uint32_t, 64> schedule{};
    for (int index = 0; index < 16; ++index) {
        const auto base = static_cast<std::size_t>(index * 4);
        schedule[index] = (static_cast<std::uint32_t>(block[base + 0]) << 24U) |
                          (static_cast<std::uint32_t>(block[base + 1]) << 16U) |
                          (static_cast<std::uint32_t>(block[base + 2]) << 8U) |
                          static_cast<std::uint32_t>(block[base + 3]);
    }

    for (int index = 16; index < 64; ++index) {
        const auto s0 = rotr(schedule[index - 15], 7) ^ rotr(schedule[index - 15], 18) ^
                        (schedule[index - 15] >> 3U);
        const auto s1 = rotr(schedule[index - 2], 17) ^ rotr(schedule[index - 2], 19) ^
                        (schedule[index - 2] >> 10U);
        schedule[index] = schedule[index - 16] + s0 + schedule[index - 7] + s1;
    }

    auto a = context.state[0];
    auto b = context.state[1];
    auto c = context.state[2];
    auto d = context.state[3];
    auto e = context.state[4];
    auto f = context.state[5];
    auto g = context.state[6];
    auto h = context.state[7];

    for (int index = 0; index < 64; ++index) {
        const auto s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const auto choice = (e & f) ^ ((~e) & g);
        const auto temp1 = h + s1 + choice + k_table[static_cast<std::size_t>(index)] + schedule[index];
        const auto s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const auto majority = (a & b) ^ (a & c) ^ (b & c);
        const auto temp2 = s0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context.state[0] += a;
    context.state[1] += b;
    context.state[2] += c;
    context.state[3] += d;
    context.state[4] += e;
    context.state[5] += f;
    context.state[6] += g;
    context.state[7] += h;
}

void update(Context& context, const std::vector<std::uint8_t>& bytes) {
    for (const auto value : bytes) {
        context.buffer[context.buffered++] = value;
        if (context.buffered == context.buffer.size()) {
            process_block(context, context.buffer.data());
            context.total_bytes += context.buffer.size();
            context.buffered = 0;
        }
    }
}

auto finalize(Context& context) -> Sha256Digest {
    context.total_bytes += context.buffered;
    const auto bit_length = context.total_bytes * 8U;

    context.buffer[context.buffered++] = 0x80;
    if (context.buffered > 56) {
        while (context.buffered < context.buffer.size()) {
            context.buffer[context.buffered++] = 0x00;
        }
        process_block(context, context.buffer.data());
        context.buffered = 0;
    }

    while (context.buffered < 56) {
        context.buffer[context.buffered++] = 0x00;
    }

    for (int shift = 7; shift >= 0; --shift) {
        context.buffer[context.buffered++] =
            static_cast<std::uint8_t>((bit_length >> (static_cast<unsigned>(shift) * 8U)) & 0xFFU);
    }

    process_block(context, context.buffer.data());

    Sha256Digest digest{};
    for (std::size_t index = 0; index < context.state.size(); ++index) {
        const auto value = context.state[index];
        const auto base = index * 4;
        digest[base + 0] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
        digest[base + 1] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
        digest[base + 2] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
        digest[base + 3] = static_cast<std::uint8_t>(value & 0xFFU);
    }
    return digest;
}

}  // namespace

auto sha256(const std::vector<std::uint8_t>& bytes) -> Sha256Digest {
    Context context;
    update(context, bytes);
    return finalize(context);
}

}  // namespace vanguard8::core
