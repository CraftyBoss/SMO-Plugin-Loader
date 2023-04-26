#pragma once

// re-impls of SMO string utils for use in non-game agnostic code
namespace StringHelper {
    bool isEqualString(const char* pString_0, const char* pString_1);

    bool isEndWithString(const char* pString_0, const char* pString_1);
}
