#pragma once
#include <cstdint>

#include "types.h"
#include "lib.hpp"

// this system sucks, casting a function pointer to void ptr means we lose whatever the signature is, so the event isn't guaranteed to match
class EventData {
public:
    template<typename Prefix, typename Postfix>
    EventData(Prefix* prefix, Postfix* postfix) : mPrefixPtr(reinterpret_cast<void*>(prefix)), mPostfixPtr(reinterpret_cast<void*>(postfix)) {}

    template<typename Prefix>
    explicit EventData(Prefix* prefix) : mPrefixPtr(reinterpret_cast<void*>(prefix)) {}

    EventData() = default;

    void* mPrefixPtr = nullptr;
    void* mPostfixPtr = nullptr;
};