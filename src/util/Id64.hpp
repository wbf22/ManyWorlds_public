#pragma once

#include <cstdint>
#include <string>

using namespace std;



struct Id64 {

    enum class IdLengthTier : uint8_t {
        TIER_0 = 8, // max 256
        TIER_1 = 13, // max 8192
        TIER_2 = 16, // max 65536
        TIER_3 = 64 // max 18446744073709551616
    };

    IdLengthTier length;
    uint64_t id;
    
    static string human_readable(uint64_t id) {
        // Group the raw decimal digits with dashes (thousands-style) for easy reading.
        string digits = to_string(id);
        string result;
        result.reserve(digits.size() + digits.size() / 3);
        size_t first = digits.size() % 3;
        if (first == 0) first = 3;
        for (size_t i = 0; i < digits.size(); ++i) {
            if (i == first || (i > first && (i - first) % 3 == 0)) {
                result += '-';
            }
            result += digits[i];
        }
        return result;
    }
};