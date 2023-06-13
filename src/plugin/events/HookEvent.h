#pragma once

#include "EventData.h"
#include "lib.hpp"
#include "types.h"

template <typename Derived>
struct EventHolderHook {
protected:
    using PrefixFuncType = bool(void);
    using PostfixFuncType = void(void);
    using Events = std::array<EventData, 100>;

    static void Callback() {
        auto [events, eventCount] = GetEvents();

        for (int i = 0; i < eventCount; ++i) {
            auto prefixFunc = reinterpret_cast<PrefixFuncType*>(events.at(i).mPrefixPtr);
            if (!prefixFunc) continue;
            if (!prefixFunc()) return;
        }

        Orig();

        for (int i = 0; i < eventCount; ++i) {
            auto postfixFunc = reinterpret_cast<PostfixFuncType*>(events.at(i).mPostfixPtr);
            if (postfixFunc)
                postfixFunc();
        }
    }

    static ALWAYS_INLINE bool AddEvent(PrefixFuncType prefixFunc, PostfixFuncType postfixFunc) {
        auto [events, eventCount] = GetEvents();
        if (eventCount >= events.max_size()) return false;
        events[eventCount++] = EventData(prefixFunc, postfixFunc);
        return true;
    }

    static ALWAYS_INLINE std::tuple<Events&, size_t&> GetEvents() {
        static Events events = {};
        static size_t eventCount = 0;
        return { events, eventCount };
    }

    static ALWAYS_INLINE void RemoveEvents() {
        auto [events, eventCount] = GetEvents();
        eventCount = 0;
    }

    // exlaunch hook code
    using CallbackFuncPtr = decltype(&Callback);

    static ALWAYS_INLINE auto& OrigRef() {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        static constinit CallbackFuncPtr s_FnPtr = nullptr;
        return s_FnPtr;
    }
    static ALWAYS_INLINE void Orig() {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef()();
    }
public:
    static ALWAYS_INLINE void InstallAtOffset(ptrdiff_t address) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(exl::util::modules::GetTargetStart() + address, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtPtr(uintptr_t ptr) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(ptr, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtSymbol(const char *sym) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        uintptr_t address = 0;
        EXL_ASSERT(nn::ro::LookupSymbol(&address, sym).isSuccess(), "Unable to find Address for Symbol: %s", sym);
        OrigRef() = exl::hook::Hook(address, Callback, true);
    }
};

template<typename Derived, typename T, typename ...Args>
struct EventHolderHookArgsR {
protected:
    using PrefixFuncType = bool(T& returnValue, Args... args);
    using PostfixFuncType = void(T& returnValue, Args... args);
    using Events = std::array<EventData, 100>;

    static T Callback(Args... args) {
        T result = {};
        auto [events, eventCount] = GetEvents();

        for (int i = 0; i < eventCount; ++i) {
            auto prefixFunc = reinterpret_cast<PrefixFuncType*>(events.at(i).mPrefixPtr);
            if (!prefixFunc) continue;
            if (!prefixFunc(result, args...)) return result;
        }

        result = Orig(std::forward<Args>(args)...);

        for (int i = 0; i < eventCount; ++i) {
            auto postfixFunc = reinterpret_cast<PostfixFuncType*>(events.at(i).mPostfixPtr);
            if (postfixFunc)
                postfixFunc(result, args...);
        }

        return result;
    }

    static ALWAYS_INLINE bool AddEvent(PrefixFuncType prefixFunc, PostfixFuncType postfixFunc) {
        auto [events, eventCount] = GetEvents();
        if (eventCount >= events.max_size()) return false;
        events[eventCount++] = EventData(prefixFunc, postfixFunc);
        return true;
    }

    static ALWAYS_INLINE std::tuple<Events&, size_t&> GetEvents() {
        static Events events = {};
        static size_t eventCount = 0;
        return { events, eventCount };
    }

    static ALWAYS_INLINE void RemoveEvents() {
        auto [events, eventCount] = GetEvents();
        eventCount = 0;
    }

    // exlaunch hook code
    using CallbackFuncPtr = decltype(&Callback);

    static ALWAYS_INLINE auto& OrigRef() {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        static constinit CallbackFuncPtr s_FnPtr = nullptr;
        return s_FnPtr;
    }
    static ALWAYS_INLINE auto Orig(Args &&... args) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        return OrigRef()(std::forward<Args>(args)...);
    }
public:
    static ALWAYS_INLINE void InstallAtOffset(ptrdiff_t address) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(exl::util::modules::GetTargetStart() + address, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtPtr(uintptr_t ptr) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(ptr, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtSymbol(const char *sym) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        uintptr_t address = 0;
        EXL_ASSERT(nn::ro::LookupSymbol(&address, sym).isSuccess(), "Unable to find Address for Symbol: %s", sym);
        OrigRef() = exl::hook::Hook(address, Callback, true);
    }
};

template<typename Derived, typename ...Args>
struct EventHolderHookArgs {
protected:
    using PrefixFuncType = bool(Args... args);
    using PostfixFuncType = void(Args... args);
    using Events = std::array<EventData, 100>;

    static void Callback(Args... args) {
        auto [events, eventCount] = GetEvents();

        for (int i = 0; i < eventCount; ++i) {
            auto prefixFunc = reinterpret_cast<PrefixFuncType*>(events.at(i).mPrefixPtr);
            if (!prefixFunc) continue;
            if (!prefixFunc(args...)) return;
        }

        Orig(std::forward<Args>(args)...);

        for (int i = 0; i < eventCount; ++i) {
            auto postfixFunc = reinterpret_cast<PostfixFuncType*>(events.at(i).mPostfixPtr);
            if (postfixFunc)
                postfixFunc(args...);
        }
    }

    static ALWAYS_INLINE bool AddEvent(PrefixFuncType prefixFunc, PostfixFuncType postfixFunc) {
        auto [events, eventCount] = GetEvents();
        if (eventCount >= events.max_size()) return false;
        events[eventCount++] = EventData(prefixFunc, postfixFunc);
        return true;
    }

    static ALWAYS_INLINE std::tuple<Events&, size_t&> GetEvents() {
        static Events events = {};
        static size_t eventCount = 0;
        return { events, eventCount };
    }

    static ALWAYS_INLINE void RemoveEvents() {
        auto [events, eventCount] = GetEvents();
        eventCount = 0;
    }

    // exlaunch hook code
    using CallbackFuncPtr = decltype(&Callback);

    static ALWAYS_INLINE auto& OrigRef() {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        static constinit CallbackFuncPtr s_FnPtr = nullptr;
        return s_FnPtr;
    }
    static ALWAYS_INLINE void Orig(Args &&... args) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef()(std::forward<Args>(args)...);
    }
public:
    static ALWAYS_INLINE void InstallAtOffset(ptrdiff_t address) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(exl::util::modules::GetTargetStart() + address, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtPtr(uintptr_t ptr) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(ptr, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtSymbol(const char *sym) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        uintptr_t address = 0;
        EXL_ASSERT(nn::ro::LookupSymbol(&address, sym).isSuccess(), "Unable to find Address for Symbol: %s", sym);
        OrigRef() = exl::hook::Hook(address, Callback, true);
    }
};

template<typename Derived, typename T>
struct EventHolderHookR {
protected:
    using PrefixFuncType = bool(T& returnValue);
    using PostfixFuncType = void(T& returnValue);
    using Events = std::array<EventData, 100>;

    static T Callback() {
        T result = {};
        auto [events, eventCount] = GetEvents();

        for (int i = 0; i < eventCount; ++i) {
            auto prefixFunc = reinterpret_cast<PrefixFuncType*>(events.at(i).mPrefixPtr);
            if (!prefixFunc) continue;
            if (!prefixFunc(result)) return result;
        }

        result = Orig();

        for (int i = 0; i < eventCount; ++i) {
            auto postfixFunc = reinterpret_cast<PostfixFuncType*>(events.at(i).mPostfixPtr);
            if (postfixFunc)
                postfixFunc(result);
        }

        return result;
    }

    static ALWAYS_INLINE bool AddEvent(PrefixFuncType prefixFunc, PostfixFuncType postfixFunc) {
        auto [events, eventCount] = GetEvents();
        if (eventCount >= events.max_size()) return false;
        events[eventCount++] = EventData(prefixFunc, postfixFunc);
        return true;
    }

    static ALWAYS_INLINE std::tuple<Events&, size_t&> GetEvents() {
        static Events events = {};
        static size_t eventCount = 0;
        return { events, eventCount };
    }

    static ALWAYS_INLINE void RemoveEvents() {
        auto [events, eventCount] = GetEvents();
        eventCount = 0;
    }

    // exlaunch hook code
    using CallbackFuncPtr = decltype(&Callback);

    static ALWAYS_INLINE auto& OrigRef() {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        static constinit CallbackFuncPtr s_FnPtr = nullptr;
        return s_FnPtr;
    }
    static ALWAYS_INLINE auto Orig() {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        return OrigRef()();
    }
public:
    static ALWAYS_INLINE void InstallAtOffset(ptrdiff_t address) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(exl::util::modules::GetTargetStart() + address, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtPtr(uintptr_t ptr) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        OrigRef() = exl::hook::Hook(ptr, Callback, true);
    }
    static ALWAYS_INLINE void InstallAtSymbol(const char *sym) {
        static_assert(!std::is_member_function_pointer_v<CallbackFuncPtr>, "Callback method must be static!");
        uintptr_t address = 0;
        EXL_ASSERT(nn::ro::LookupSymbol(&address, sym).isSuccess(), "Unable to find Address for Symbol: %s", sym);
        OrigRef() = exl::hook::Hook(address, Callback, true);
    }
};