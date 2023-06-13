#pragma once

#include "logger/Logger.hpp"
#include <cstring>

// pretty much identical to skylines impl
struct Sha256Hash {
    u8 data[0x20] = {};

    bool operator==(const Sha256Hash &rs) const { return memcmp(this->data, rs.data, sizeof(this->data)) == 0; }
    bool operator!=(const Sha256Hash &rs) const { return !operator==(rs); }

    bool operator<(const Sha256Hash &rs) const { return memcmp(this->data, rs.data, sizeof(this->data)) < 0; }
    bool operator>(const Sha256Hash &rs) const { return memcmp(this->data, rs.data, sizeof(this->data)) > 0; }

    void print() {
        for (u8 i : data) { Logger::log("%02X", i); }
        Logger::log("\n");
    }

    void sprint(char* s) {
        for (int i = 0; i < 0x20; i++) {
            u8 val = data[i];
            sprintf(s + (i * 2), "%02X", val);
        }
    }
};