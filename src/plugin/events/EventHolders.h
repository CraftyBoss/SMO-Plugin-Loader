#pragma once

#include "EventData.h"
#include "HookEvent.h"
#include "ModEvent.h"

#define COMPILE_RULES EXPORT_SYM NOINLINE ATTRIB_USED

#define CREATE_HOOK_EVENT(Name, HookVal)                                                                                                     \
struct Name : public EventHolderHook<Name> {                                                                                                 \
    static constexpr auto InstallVal = HookVal;                                                                                              \
    static COMPILE_RULES bool addEvent(EventHolderHook<Name>::PrefixFuncType prefixFunc, EventHolderHook<Name>::PostfixFuncType postfixFunc) \
        { return EventHolderHook<Name>::AddEvent(prefixFunc, postfixFunc); }                                                                 \
    static void clearEvents() { EventHolderHook<Name>::RemoveEvents(); }                                                                     \
};

#define CREATE_HOOK_EVENT_ARGSR(Name, HookVal, ReturnType, ...)                                                              \
struct Name : public EventHolderHookArgsR<Name, ReturnType, __VA_ARGS__> {                                                   \
    static constexpr auto InstallVal = HookVal;                                                                              \
    static COMPILE_RULES bool addEvent(EventHolderHookArgsR<Name, ReturnType, __VA_ARGS__>::PrefixFuncType prefixFunc,       \
                                           EventHolderHookArgsR<Name, ReturnType, __VA_ARGS__>::PostfixFuncType postfixFunc) \
        { return EventHolderHookArgsR<Name, ReturnType, __VA_ARGS__>::AddEvent(prefixFunc, postfixFunc); }                   \
    static void clearEvents() { EventHolderHookArgsR<Name, ReturnType, __VA_ARGS__>::RemoveEvents(); }                       \
};

#define CREATE_HOOK_EVENT_ARGS(Name, HookVal, ...)                                                                                                                              \
struct Name : public EventHolderHookArgs<Name, __VA_ARGS__> {                                                                                                                   \
    static constexpr auto InstallVal = HookVal;                                                                                                                                 \
    static COMPILE_RULES bool addEvent(EventHolderHookArgs<Name, __VA_ARGS__>::PrefixFuncType prefixFunc, EventHolderHookArgs<Name, __VA_ARGS__>::PostfixFuncType postfixFunc)  \
        { return EventHolderHookArgs<Name, __VA_ARGS__>::AddEvent(prefixFunc, postfixFunc); }                                                                                   \
    static void clearEvents() { EventHolderHookArgs<Name, __VA_ARGS__>::RemoveEvents(); }                                                                                       \
};

#define CREATE_HOOK_EVENT_R(Name, HookVal, ReturnType) \
struct Name : public EventHolderHookR<Name, ReturnType> { \
    static constexpr auto InstallVal = HookVal;                                             \
    static COMPILE_RULES bool addEvent(EventHolderHookR<Name, ReturnType>::PrefixFuncType prefixFunc, EventHolderHookR<Name, ReturnType>::PostfixFuncType postfixFunc) \
        { return EventHolderHookR<Name, ReturnType>::AddEvent(prefixFunc, postfixFunc); }          \
    static void clearEvents() { EventHolderHookR<Name, ReturnType>::RemoveEvents(); }                                                       \
};

#define CREATE_MOD_EVENT(Name) \
struct Name : public EventHolderMod { \
    static COMPILE_RULES bool addEvent(EventHolderMod::BaseEventFunc event) { return EventHolderMod::AddEvent(event); } \
    static void clearEvents() { EventHolderMod::RemoveAllEvents(); }                                                    \
    static void removeEvents(const char* key) { EventHolderMod::RemoveEvents(key); }                                    \
};

#define CREATE_MOD_EVENT_ARGSR(Name, ReturnType, ...) \
struct Name : public EventHolderModArgsR<ReturnType, __VA_ARGS__> { \
    static COMPILE_RULES bool addEvent(EventHolderModArgsR<ReturnType, __VA_ARGS__>::BaseEventFunc& event) { return EventHolderModArgsR<ReturnType, __VA_ARGS__>::AddEvent(event); } \
    static void clearEvents() { EventHolderModArgsR<ReturnType, __VA_ARGS__>::RemoveAllEvents(); }                                                      \
};

#define CREATE_MOD_EVENT_ARGS(Name, ...) \
struct Name : public EventHolderModArgs<__VA_ARGS__> { \
    static COMPILE_RULES bool addEvent(EventHolderModArgs<__VA_ARGS__>::BaseEventFunc& event) { return EventHolderModArgs<__VA_ARGS__>::AddEvent(event); } \
    static void clearEvents() { EventHolderModArgs<__VA_ARGS__>::RemoveAllEvents(); }                                         \
};

#define CREATE_MOD_EVENT_R(Name, ReturnType) \
struct Name : public EventHolderModR<ReturnType> { \
    static COMPILE_RULES bool addEvent(EventHolderModR<ReturnType>::BaseEventFunc& event) { return EventHolderModR<ReturnType>::AddEvent(event); } \
    static void clearEvents() { EventHolderModR<ReturnType>::RemoveAllEvents(); }                                             \
};

#define INSTALL_EVENT(Name, Type) \
Name::InstallAt##Type(Name::InstallVal)