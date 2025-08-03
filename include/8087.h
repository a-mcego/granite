#pragma once

#include <cstdint>
#include <array>

using u32 = std::uint32_t;
using u8 = std::uint8_t;

template<u32 EXPONENT, u32 MANTISSA>
struct Float
{
    static_assert(EXPONENT > 0 && EXPONENT <= 16, "Exponent bits must be 1-16");
    static_assert(MANTISSA > 0 && MANTISSA <= 68, "Mantissa bits must be 1-68");

    // Total bits: sign + exponent + mantissa
    static constexpr u32 TOTAL_BITS = 1 + EXPONENT + MANTISSA;

    // Number of bytes needed for packed representation
    static constexpr u32 PACKED_BYTES = (TOTAL_BITS + 7) / 8;

    // Number of u32s needed to store mantissa bits
    static constexpr u32 MANTISSA_STORAGE_SIZE = (MANTISSA + 31) / 32;

    using mantissa_type = std::array<u32, MANTISSA_STORAGE_SIZE>;
    using packed_type = std::array<u8, PACKED_BYTES>;

    bool sign{};
    u32 exponent{};
    mantissa_type mantissa{};

    // Pack into byte array (LSB order)
    packed_type pack() const
    {
        packed_type result{};
        u32 bit_pos = 0;

        // Pack mantissa first (LSB)
        for (u32 i = 0; i < MANTISSA; ++i)
        {
            u32 word_idx = i / 32;
            u32 bit_in_word = i % 32;
            bool bit = (mantissa[word_idx] >> bit_in_word) & 1;

            if (bit)
            {
                result[bit_pos / 8] |= (1 << (bit_pos % 8));
            }
            ++bit_pos;
        }

        // Pack exponent
        for (u32 i = 0; i < EXPONENT; ++i)
        {
            bool bit = (exponent >> i) & 1;
            if (bit)
            {
                result[bit_pos / 8] |= (1 << (bit_pos % 8));
            }
            ++bit_pos;
        }

        // Pack sign (MSB)
        if (sign)
        {
            result[bit_pos / 8] |= (1 << (bit_pos % 8));
        }

        return result;
    }

    // Unpack from byte array (LSB order)
    void unpack(const packed_type& data)
    {
        mantissa = {};
        exponent = 0;
        sign = false;

        u32 bit_pos = 0;

        // Unpack mantissa first (LSB)
        for (u32 i = 0; i < MANTISSA; ++i)
        {
            bool bit = (data[bit_pos / 8] >> (bit_pos % 8)) & 1;
            if (bit)
            {
                u32 word_idx = i / 32;
                u32 bit_in_word = i % 32;
                mantissa[word_idx] |= (1u << bit_in_word);
            }
            ++bit_pos;
        }

        // Unpack exponent
        for (u32 i = 0; i < EXPONENT; ++i) {
            bool bit = (data[bit_pos / 8] >> (bit_pos % 8)) & 1;
            if (bit)
            {
                exponent |= (1u << i);
            }
            ++bit_pos;
        }

        // Unpack sign (MSB)
        sign = (data[bit_pos / 8] >> (bit_pos % 8)) & 1;
    }
};
