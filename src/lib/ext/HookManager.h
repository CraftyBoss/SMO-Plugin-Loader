#pragma once
// exl_ext is the extension namespace for all exl related classes
// some nasty stuff here (also none of this works so don't look at it)
#include "types.h"
#include <functional>
#include <hook/base.hpp>
#include <map>
#include <type_traits>

#define GetCurFuncAddr(Val)                                                                               \
{                                                                                                         \
stack_frame *frame;                                                                                       \
asm("mov %0, fp" : "=r"(frame));                                                                          \
MemoryInfo memInfo;                                                                                       \
u32 pageInfo;                                                                                             \
if (R_FAILED(svcQueryMemory(&memInfo, &pageInfo, (u64)frame)) || (memInfo.perm & Perm_R) == 0) {Val = 0;} \
Val = frame->lr;                                                                                          \
}

namespace exl_ext::hook {

    typedef bool (*PrefixFunc)();
    typedef void (*PostfixFunc)(void*);


    template<typename R, typename ...Args>
    struct function_traits
    {
        static const size_t nargs = sizeof...(Args);

        typedef R result_type;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    // header for plugin
    template <typename Prefix, typename Postfix>
    struct IPluginPatch {
        using PatchType = IPluginPatch<Prefix, Postfix>;
        using Traits = function_traits<Prefix>;

        typedef Traits::result_type OrigType;

        static void (*sOrigPtr)();
        static Prefix sPrefixFunc;
        static Postfix sPostfixFunc;

        static void Callback() {
            void *result = nullptr;

            if(PatchType::sPrefixFunc()) {
                auto func = reinterpret_cast<OrigType>(sOrigPtr);
                func();
            }

            PatchType::sPostfixFunc(result);
        }
    };

    template <typename Prefix, typename Postfix>
    void (*IPluginPatch<Prefix, Postfix>::sOrigPtr)() = nullptr;
    template <typename Prefix, typename Postfix>
    Prefix IPluginPatch<Prefix, Postfix>::sPrefixFunc = nullptr;
    template <typename Prefix, typename Postfix>
    Postfix IPluginPatch<Prefix, Postfix>::sPostfixFunc = nullptr;

    class HookManager {
    public:
        template <typename Prefix, typename Postfix>
        static void RegisterAtOffset(uintptr_t ptr, Prefix prefix, Postfix postfix) {
            using PatchType = IPluginPatch<Prefix, Postfix>;

            PatchType::sPrefixFunc = prefix;
            PatchType::sPostfixFunc = postfix;

            PatchType::sOrigPtr = exl::hook::Hook(ptr, PatchType::Callback, true);
        }
    };
}

#define Create_Trampoline_Patch(Symbol, Name)   \
class Name : exl_ext::hook::IPluginPatch {      \
        using CallbackType = decltype(&Symbol); \
};