// From: https://github.com/rawrunprotected/hilbert_curves/blob/master/hilbert_curves.cpp (public domain)

#pragma once

#include <cstdint>

namespace ninedb::detail
{
    uint32_t deinterleave(uint32_t x)
    {
        x = x & 0x55555555;
        x = (x | (x >> 1)) & 0x33333333;
        x = (x | (x >> 2)) & 0x0F0F0F0F;
        x = (x | (x >> 4)) & 0x00FF00FF;
        x = (x | (x >> 8)) & 0x0000FFFF;
        return x;
    }

    uint32_t interleave(uint32_t x)
    {
        x = (x | (x << 8)) & 0x00FF00FF;
        x = (x | (x << 4)) & 0x0F0F0F0F;
        x = (x | (x << 2)) & 0x33333333;
        x = (x | (x << 1)) & 0x55555555;
        return x;
    }

    uint32_t prefix_scan(uint32_t x)
    {
        x = (x >> 8) ^ x;
        x = (x >> 4) ^ x;
        x = (x >> 2) ^ x;
        x = (x >> 1) ^ x;
        return x;
    }

    uint32_t descan(uint32_t x)
    {
        return x ^ (x >> 1);
    }

    void hilbert_index_to_xy(uint32_t n, uint32_t i, uint32_t &x, uint32_t &y)
    {
        i = i << (32 - 2 * n);

        uint32_t i0 = deinterleave(i);
        uint32_t i1 = deinterleave(i >> 1);

        uint32_t t0 = (i0 | i1) ^ 0xFFFF;
        uint32_t t1 = i0 & i1;

        uint32_t prefixT0 = prefix_scan(t0);
        uint32_t prefixT1 = prefix_scan(t1);

        uint32_t a = (((i0 ^ 0xFFFF) & prefixT1) | (i0 & prefixT0));

        x = (a ^ i1) >> (16 - n);
        y = (a ^ i0 ^ i1) >> (16 - n);
    }

    // Map a point in the d=2-dimensional plane to its corresponding d*n-bit offset on the n-th order Hilbert curve.
    uint32_t hilbert_xy_to_index(uint32_t n, uint32_t x, uint32_t y)
    {
        x = x << (16 - n);
        y = y << (16 - n);

        uint32_t A, B, C, D;

        // Initial prefix scan round, prime with x and y
        {
            uint32_t a = x ^ y;
            uint32_t b = 0xFFFF ^ a;
            uint32_t c = 0xFFFF ^ (x | y);
            uint32_t d = x & (y ^ 0xFFFF);

            A = a | (b >> 1);
            B = (a >> 1) ^ a;

            C = ((c >> 1) ^ (b & (d >> 1))) ^ c;
            D = ((a & (c >> 1)) ^ (d >> 1)) ^ d;
        }

        {
            uint32_t a = A;
            uint32_t b = B;
            uint32_t c = C;
            uint32_t d = D;

            A = ((a & (a >> 2)) ^ (b & (b >> 2)));
            B = ((a & (b >> 2)) ^ (b & ((a ^ b) >> 2)));

            C ^= ((a & (c >> 2)) ^ (b & (d >> 2)));
            D ^= ((b & (c >> 2)) ^ ((a ^ b) & (d >> 2)));
        }

        {
            uint32_t a = A;
            uint32_t b = B;
            uint32_t c = C;
            uint32_t d = D;

            A = ((a & (a >> 4)) ^ (b & (b >> 4)));
            B = ((a & (b >> 4)) ^ (b & ((a ^ b) >> 4)));

            C ^= ((a & (c >> 4)) ^ (b & (d >> 4)));
            D ^= ((b & (c >> 4)) ^ ((a ^ b) & (d >> 4)));
        }

        // Final round and projection
        {
            uint32_t a = A;
            uint32_t b = B;
            uint32_t c = C;
            uint32_t d = D;

            C ^= ((a & (c >> 8)) ^ (b & (d >> 8)));
            D ^= ((b & (c >> 8)) ^ ((a ^ b) & (d >> 8)));
        }

        // Undo transformation prefix scan
        uint32_t a = C ^ (C >> 1);
        uint32_t b = D ^ (D >> 1);

        // Recover index bits
        uint32_t i0 = x ^ y;
        uint32_t i1 = b | (0xFFFF ^ (i0 | a));

        return ((interleave(i1) << 1) | interleave(i0)) >> (32 - 2 * n);
    }

    // These are multiplication tables of the alternating group A4,
    // preconvolved with the mapping between Morton and Hilbert curves.
    static const uint8_t morton_to_hilbert_table[] = {
        48,
        33,
        27,
        34,
        47,
        78,
        28,
        77,
        66,
        29,
        51,
        52,
        65,
        30,
        72,
        63,
        76,
        95,
        75,
        24,
        53,
        54,
        82,
        81,
        18,
        3,
        17,
        80,
        61,
        4,
        62,
        15,
        0,
        59,
        71,
        60,
        49,
        50,
        86,
        85,
        84,
        83,
        5,
        90,
        79,
        56,
        6,
        89,
        32,
        23,
        1,
        94,
        11,
        12,
        2,
        93,
        42,
        41,
        13,
        14,
        35,
        88,
        36,
        31,
        92,
        37,
        87,
        38,
        91,
        74,
        8,
        73,
        46,
        45,
        9,
        10,
        7,
        20,
        64,
        19,
        70,
        25,
        39,
        16,
        69,
        26,
        44,
        43,
        22,
        55,
        21,
        68,
        57,
        40,
        58,
        67,
    };

    static const uint8_t hilbert_to_morton_table[] = {
        48,
        33,
        35,
        26,
        30,
        79,
        77,
        44,
        78,
        68,
        64,
        50,
        51,
        25,
        29,
        63,
        27,
        87,
        86,
        74,
        72,
        52,
        53,
        89,
        83,
        18,
        16,
        1,
        5,
        60,
        62,
        15,
        0,
        52,
        53,
        57,
        59,
        87,
        86,
        66,
        61,
        95,
        91,
        81,
        80,
        2,
        6,
        76,
        32,
        2,
        6,
        12,
        13,
        95,
        91,
        17,
        93,
        41,
        40,
        36,
        38,
        10,
        11,
        31,
        14,
        79,
        77,
        92,
        88,
        33,
        35,
        82,
        70,
        10,
        11,
        23,
        21,
        41,
        40,
        4,
        19,
        25,
        29,
        47,
        46,
        68,
        64,
        34,
        45,
        60,
        62,
        71,
        67,
        18,
        16,
        49,
    };

    uint32_t transform_curve(uint32_t in, uint32_t bits, const uint8_t *lookup_table)
    {
        uint32_t transform = 0;
        uint32_t out = 0;

        for (int32_t i = 3 * (bits - 1); i >= 0; i -= 3)
        {
            transform = lookup_table[transform | ((in >> i) & 7)];
            out = (out << 3) | (transform & 7);
            transform &= ~7;
        }

        return out;
    }

    uint32_t morton_to_hilbert_3d(uint32_t morton_index, uint32_t bits)
    {
        return transform_curve(morton_index, bits, morton_to_hilbert_table);
    }

    uint32_t hilbert_to_morton_3d(uint32_t hilbert_index, uint32_t bits)
    {
        return transform_curve(hilbert_index, bits, hilbert_to_morton_table);
    }
}
