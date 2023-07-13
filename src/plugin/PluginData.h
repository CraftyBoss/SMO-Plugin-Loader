#pragma once

#include "plugin/LoaderCtx.h"
#include "sha256.h"

#include <nn/ro.h>

class PluginData {
    template <typename T>
    bool getPluginFunc(T& funcPtr, const char* symbol) {
        if(!mModuleLoaded) {
            Logger::log("Plugin module not Loaded! Cannot lookup symbol: %s\n", symbol);
            return false;
        }

        nn::Result result = nn::ro::LookupModuleSymbol(reinterpret_cast<uintptr_t*>(&funcPtr), &mModule, symbol);
        if(result.isSuccess()) {
            return true;
        }
        Logger::log("Unable to find ptr for symbol: %s\n", symbol);
        return false;
    }

    // plugin func signatures
    typedef bool (*PluginMain)(LoaderCtx& ctx);

public:
    char mFilePath[0x40] = {};
    char mFileName[0x30] = {};
    Sha256Hash mPluginHash = {};

    u8* mFileData = nullptr;
    size_t mFileSize = 0;

    nn::ro::Module mModule = {};
    bool mModuleLoaded = false;

    u8* mBssData = nullptr;
    size_t mBssSize = 0;

    sead::Heap* mHeap = nullptr;

    bool runPluginMain(LoaderCtx& ctx);

};
