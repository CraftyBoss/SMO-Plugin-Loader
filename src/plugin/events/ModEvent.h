#pragma once

#include "EventData.h"
#include "lib.hpp"
#include "types.h"
#include <container/seadStrTreeMap.h>
#include <container/seadTreeMap.h>
#include <heap/seadHeapMgr.h>
#include <exception/ExceptionHandler.h>
#include <map>
#include <os/os_thread_api.hpp>
#include <plugin/PluginLoader.h>

// Event Holder types for events that are triggered by the main plugin loader (ex: debug drawing)

// EventHolderMod      - Event Holder for events with no return type and no arguments
// EventHolderModArgsR - Event holder for events with a return type and arguments
// EventHolderModArgs  - Event holder for events with no return type and arguments
// EventHolderModR     - Event holder for events with a return type and no arguments
// Note: this isn't the most preferable solution to how I'd like to go about this, but it works well enough for now.

// helper func used to get the func addr of a std::function
template<typename T, typename... U>
static ALWAYS_INLINE size_t getEventAddress(std::function<T(U...)> f) {
    typedef T(fnType)(U...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}

// FIXME: ALl events that inherit from this will use the same static method, instead of a unique one
template <typename Derived>
struct EventHolderMod {
protected:
    using BaseEventFunc = void();
    using EventContainer = std::vector<std::function<BaseEventFunc>>;
    using Events = sead::StrTreeMap<64, EventContainer>;

    static ALWAYS_INLINE bool AddEvent(BaseEventFunc eventFunc) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        uintptr_t addr = *(reinterpret_cast<uintptr_t*>(&eventFunc));

        char moduleName[0x100] = {};
        if(handler::getAddrModuleName(moduleName, addr)) {
            Logger::log("Got event from Plugin: %s\n", moduleName);
        }else {
            Logger::log("Unable to find Plugin Module Info.\n");
            return false;
        }

        if(!eventMap.find(moduleName)) {
            eventMap.insert(moduleName, EventContainer());
        }

        eventMap.find(moduleName)->value().emplace_back(eventFunc);

        return true;
    }

    static ALWAYS_INLINE Events& GetEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        static Events events = {};
        static bool isAlloc = false;

        if(!isAlloc) {
            events.allocBuffer(100, nullptr); // uses current scope heap
            isAlloc = true;
        }

        return events;
    }

public:
    static void RunEvents() {
        auto& eventMap = GetEvents();
        eventMap.forEach([](auto& key, auto& value) {
            for (auto& eventFunc : value) {
                if (!eventFunc)
                    continue;
                eventFunc();
            }
        });

        // TODO: return from exception once proper plugin unloading works
//        handler::tryCatch(
//            []() {
//                static std::function<BaseEventFunc> curEventFunc = nullptr; // TODO: fix scope capturing not working with exception handler
//            },
//            [](handler::ExceptionInfo& info) {
//                Logger::log("Exception caught while running event.\n");
//                handler::printStackTrace(info, 1, true);
//
//                uintptr_t addr = getEventAddress(curEventFunc);
//                char moduleName[0x100] = {};
//                if(handler::getAddrModuleName(moduleName, addr)) {
//                    PluginLoader::unloadPluginByName(moduleName);
//                    Logger::log("Unloaded Plugin. Returning to event execution.\n");
//                }else {
//                    Logger::log("Unable to find Plugin Module Info.\n");
//                }
//
//                return false;
//            }
//        );
    }

    static void RemoveAllEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& events = GetEvents();
        events.clear();

        // reset buffer
        events.freeBuffer();
        events.allocBuffer(100, nullptr);
    }

    static void RemoveEvents(const char* key) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return;
        }

        eventMap.erase(key); // remove whole node
    }

    static bool RemoveEvent(const char* key, int idx) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return false;
        }
        EventContainer &events = eventMap.find(key)->value();

        if(events.size() > idx) {
            events.erase(events.begin() + idx);
            Logger::log("Removed event at Index: %d\n", idx);
            return true;
        }else {
            Logger::log("Attempting to remove event out of bounds!\n");
            return false;
        }
    }
};

template<typename Derived, typename T, typename ...Args>
struct EventHolderModArgsR {
protected:
    using BaseEventFunc = bool(T& returnValue, Args... args);
    using EventContainer = std::vector<std::function<BaseEventFunc>>;
    using Events = sead::StrTreeMap<64, EventContainer>;

    static ALWAYS_INLINE bool AddEvent(BaseEventFunc eventFunc) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        uintptr_t addr = *(reinterpret_cast<uintptr_t*>(&eventFunc));

        char moduleName[0x100] = {};
        if(handler::getAddrModuleName(moduleName, addr)) {
            Logger::log("Got event from Plugin: %s\n", moduleName);
        }else {
            Logger::log("Unable to find Plugin Module Info.\n");
            return false;
        }

        if(!eventMap.find(moduleName)) {
            eventMap.insert(moduleName, EventContainer());
        }

        eventMap.find(moduleName)->value().emplace_back(eventFunc);

        return true;
    }

    static ALWAYS_INLINE Events& GetEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        static Events events = {};
        static bool isAlloc = false;

        if(!isAlloc) {
            events.allocBuffer(100, nullptr); // uses current scope heap
            isAlloc = true;
        }

        return events;
    }
public:
    // TODO: add exception catching (wont work here until exception handler supports variable capturing in lambdas
    static T RunEvents(Args... args) {
        T result = {};
        auto& eventMap = GetEvents();
        eventMap.forEach([result, args...](auto& key, auto& value) {
            for (auto& eventFunc : value) {
                if (!eventFunc)
                    continue;
                if(!eventFunc(result, args...))
                    return result;
            }
        });

        return result;
    }

    static void RemoveAllEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& events = GetEvents();
        events.clear();

        // reset buffer
        events.freeBuffer();
        events.allocBuffer(100, nullptr);
    }

    static void RemoveEvents(const char* key) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return;
        }

        eventMap.erase(key); // remove whole node
    }

    static bool RemoveEvent(const char* key, int idx) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return false;
        }
        EventContainer &events = eventMap.find(key)->value();

        if(events.size() > idx) {
            events.erase(events.begin() + idx);
            Logger::log("Removed event at Index: %d\n", idx);
            return true;
        }else {
            Logger::log("Attempting to remove event out of bounds!\n");
            return false;
        }
    }
};

template<typename Derived, typename ...Args>
struct EventHolderModArgs {
protected:
    using BaseEventFunc = void(Args... args);
    using EventContainer = std::vector<std::function<BaseEventFunc>>;
    using Events = sead::StrTreeMap<64, EventContainer>;

    static ALWAYS_INLINE bool AddEvent(BaseEventFunc eventFunc) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        uintptr_t addr = *(reinterpret_cast<uintptr_t*>(&eventFunc));

        char moduleName[0x100] = {};
        if(handler::getAddrModuleName(moduleName, addr)) {
            Logger::log("Got event from Plugin: %s\n", moduleName);
        }else {
            Logger::log("Unable to find Plugin Module Info.\n");
            return false;
        }

        if(!eventMap.find(moduleName)) {
            eventMap.insert(moduleName, EventContainer());
        }

        eventMap.find(moduleName)->value().emplace_back(eventFunc);

        return true;
    }

    static ALWAYS_INLINE Events& GetEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        static Events events = {};
        static bool isAlloc = false;

        if(!isAlloc) {
            events.allocBuffer(100, nullptr); // uses current scope heap
            isAlloc = true;
        }

        return events;
    }

public:
    static void RunEvents(Args... args) {
        auto& eventMap = GetEvents();
        eventMap.forEach([args...](auto& key, auto& value) {
            for (auto& eventFunc : value) {
                if (!eventFunc)
                    continue;
                eventFunc(args...);
            }
        });
    }

    static void RemoveAllEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& events = GetEvents();
        events.clear();

        // reset buffer
        events.freeBuffer();
        events.allocBuffer(100, nullptr);
    }

    static void RemoveEvents(const char* key) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return;
        }

        eventMap.erase(key); // remove whole node
    }

    static bool RemoveEvent(const char* key, int idx) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return false;
        }
        EventContainer &events = eventMap.find(key)->value();

        if(events.size() > idx) {
            events.erase(events.begin() + idx);
            Logger::log("Removed event at Index: %d\n", idx);
            return true;
        }else {
            Logger::log("Attempting to remove event out of bounds!\n");
            return false;
        }
    }

};

template<typename Derived, typename R>
struct EventHolderModR {
protected:
    using BaseEventFunc = void(R& returnValue);
    using EventContainer = std::vector<std::function<BaseEventFunc>>;
    using Events = sead::StrTreeMap<64, EventContainer>;

    static ALWAYS_INLINE bool AddEvent(BaseEventFunc eventFunc) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        uintptr_t addr = *(reinterpret_cast<uintptr_t*>(&eventFunc));

        char moduleName[0x100] = {};
        if(handler::getAddrModuleName(moduleName, addr)) {
            Logger::log("Got event from Plugin: %s\n", moduleName);
        }else {
            Logger::log("Unable to find Plugin Module Info.\n");
            return false;
        }

        if(!eventMap.find(moduleName)) {
            eventMap.insert(moduleName, EventContainer());
        }

        eventMap.find(moduleName)->value().emplace_back(eventFunc);

        return true;
    }

    static ALWAYS_INLINE Events& GetEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        static Events events = {};
        static bool isAlloc = false;

        if(!isAlloc) {
            events.allocBuffer(100, nullptr); // uses current scope heap
            isAlloc = true;
        }

        return events;
    }

public:

    // TODO: add exception catching (wont work here until exception handler supports variable capturing in lambdas
    static R RunEvents() {
        R result = {};
        auto& eventMap = GetEvents();
        eventMap.forEach([result](auto& key, auto& value) {
            for (auto& eventFunc : value) {
                if (!eventFunc)
                    continue;
                if(!eventFunc(result))
                    return result;
            }
        });
        return result;
    }

    static void RemoveAllEvents() {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& events = GetEvents();
        events.clear();

        // reset buffer
        events.freeBuffer();
        events.allocBuffer(100, nullptr);
    }

    static void RemoveEvents(const char* key) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return;
        }

        eventMap.erase(key); // remove whole node
    }

    static bool RemoveEvent(const char* key, int idx) {
        sead::ScopedCurrentHeapSetter setter(PluginLoader::getHeap());
        auto& eventMap = GetEvents();

        if(!eventMap.find(key)) {
            Logger::log("Unable to find events. Key: %s\n", key);
            return false;
        }
        EventContainer &events = eventMap.find(key)->value();

        if(events.size() > idx) {
            events.erase(events.begin() + idx);
            Logger::log("Removed event at Index: %d\n", idx);
            return true;
        }else {
            Logger::log("Attempting to remove event out of bounds!\n");
            return false;
        }
    }


};