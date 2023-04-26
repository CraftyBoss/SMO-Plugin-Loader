#include <cstring>
#include <helpers/StringHelper.h>

namespace StringHelper {
    bool isEqualString(const char* pString_0, const char* pString_1) {
        while (*pString_0 == *pString_1) {
            char val = *pString_0;

            if (!val)
                return true;

            ++pString_1;
            ++pString_0;
        }

        return false;
    }

    bool isEndWithString(const char* pString_0, const char* pString_1) {
        size_t pString0_Len = strlen(pString_0);
        size_t pString1_Len = strlen(pString_1);

        if (pString0_Len < pString1_Len)
            return false;

        return isEqualString(&pString_0[pString0_Len - pString1_Len], pString_1);
    }
}