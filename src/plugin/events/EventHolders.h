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
struct Name : public EventHolderMod<Name> { \
    static COMPILE_RULES bool addEvent(EventHolderMod<Name>::BaseEventFunc event) { return EventHolderMod<Name>::AddEvent(event); } \
};

#define CREATE_MOD_EVENT_ARGSR(Name, ReturnType, ...) \
struct Name : public EventHolderModArgsR<Name, ReturnType, __VA_ARGS__> { \
    static COMPILE_RULES bool addEvent(EventHolderModArgsR<Name, ReturnType, __VA_ARGS__>::BaseEventFunc& event) { return EventHolderModArgsR<Name, ReturnType, __VA_ARGS__>::AddEvent(event); } \
};

#define CREATE_MOD_EVENT_ARGS(Name, ...) \
struct Name : public EventHolderModArgs<Name, __VA_ARGS__> { \
    static COMPILE_RULES bool addEvent(EventHolderModArgs<Name, __VA_ARGS__>::BaseEventFunc& event) { return EventHolderModArgs<Name, __VA_ARGS__>::AddEvent(event); }  \
};

#define CREATE_MOD_EVENT_R(Name, ReturnType) \
struct Name : public EventHolderModR<Name, ReturnType> { \
    static COMPILE_RULES bool addEvent(EventHolderModR<Name, ReturnType>::BaseEventFunc& event) { return EventHolderModR<Name, ReturnType>::AddEvent(event); }  \
};

#define INSTALL_EVENT(Name, Type) \
Name::InstallAt##Type(Name::InstallVal)