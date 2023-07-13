#include "lib.hpp"
#include <heap/seadHeapMgr.h>
#include <plugin/PluginLoader.h>
#include <plugin/events/Events.h>

#include "nn/init.h"

PluginLoader& PluginLoader::instance() {
    static PluginLoader sInstance;
    return sInstance;
}

EXPORT_SYM void* pluginAlloc(size_t size, s32 alignment = 8) {
//    return nn::init::GetAllocator()->Allocate(ALIGN_UP(size, alignment));
    return PluginLoader::getHeap()->alloc(size, alignment);
}

EXPORT_SYM void pluginFree(void* ptr) {
//    nn::init::GetAllocator()->Free(ptr);
    PluginLoader::getHeap()->free(ptr);
}

EXPORT_SYM void* pluginRealloc(void* ptr, size_t size) {
//    return nn::init::GetAllocator()->Reallocate(ptr, size);
    return PluginLoader::getHeap()->tryRealloc(ptr, size, 8);
}

bool PluginLoader::createPluginData(PluginData& data, const FsHelper::DirFileEntry& entry) {

    Logger::log("Size: %u\n", entry.bufSize);

    data.mFileSize = entry.bufSize;
    data.mFileData = (u8*)pluginAlloc(data.mFileSize, 0x1000); // must be aligned
    memcpy(data.mFileData, entry.fileBuffer, data.mFileSize);

    const char* fileName = FsHelper::getFileName(entry.fullPath);
    if(strlen(fileName) < sizeof(data.mFileName)) {
        strcpy(data.mFileName, fileName);
    }else {
        Logger::log("File name too long for buffer! Cannot copy.\n");
    }

    size_t filePathLen = strlen(entry.fullPath) - (strlen(data.mFileName) + 1);
    if(filePathLen < sizeof(data.mFilePath)) {
        strncpy(data.mFilePath, entry.fullPath, filePathLen);
        data.mFilePath[filePathLen] = '\0'; // strncpy doesn't null terminate if max length is less than full path length
    }else {
        Logger::log("File path too long for buffer! Cannot copy.\n");
    }

    if(nn::ro::GetBufferSize(&data.mBssSize, data.mFileData).isFailure()) {
        Logger::log("Failed to get NRO Buffer Size! NRO may not be valid!\n");
        pluginFree(data.mFileData);
        return false;
    }

    Logger::log("NRO Buffer size: %d\n", data.mBssSize);

    data.mBssData = (u8*)pluginAlloc(data.mBssSize, 0x1000);

    // hash header for NRR registration later
    auto* nroHeader = (nn::ro::NroHeader*)data.mFileData;
    nn::crypto::GenerateSha256Hash(&data.mPluginHash, sizeof(data.mPluginHash), nroHeader, nroHeader->size);
    
    Logger::log("NRO Hash: ");
    data.mPluginHash.print();

    // register hash to set

    if(mSortedHashes.find(data.mPluginHash) != nullptr) {
        Logger::log("Plugin has already been registered! Skipping.\n");
        pluginFree(data.mFileData);
        return false;
    }

    mSortedHashes.insert(data.mPluginHash);

    return true;
}

void PluginLoader::preparePluginsForLoad(s32 nroCount, FsHelper::DirFileEntry* fileData) {
    // create plugin buffer
    mPlugins = (PluginData*)pluginAlloc(sizeof(PluginData) * nroCount);

    mOrderedSetBuffer = pluginAlloc(sizeof(sead::OrderedSet<Sha256Hash>::Node) * nroCount);
    mSortedHashes.setBuffer(nroCount, mOrderedSetBuffer);

    // create PluginData and register NRO hash to hash set
    for (int i = 0; i < nroCount; ++i) {
        FsHelper::DirFileEntry &entry = fileData[i];

        Logger::log("Loading Plugin at Path: %s\n", entry.fullPath);

        if(!createPluginData(mPlugins[mPluginCount], entry)) {
            Logger::log("Unable to fully load Plugin.\n");
        }else {
            Logger::log("Loaded Plugin data!\n");
            mPluginCount++;
        }

        FsHelper::freeFile(entry);
    }

    FsHelper::freeEntries(fileData);

}

void PluginLoader::generatePluginNrr() {
    mNrrBufferSize = ALIGN_UP(sizeof(nn::ro::NrrHeader) + (mPluginCount * sizeof(Sha256Hash)), 0x1000);
    mNrrBuffer = (u8*)pluginAlloc( mNrrBufferSize, 0x1000);
    memset(mNrrBuffer, 0, mNrrBufferSize); // clear buffer out completely

    auto* header = reinterpret_cast<nn::ro::NrrHeader*>(mNrrBuffer);

    *header = nn::ro::NrrHeader {
        .magic = 0x3052524E,
        .program_id = {exl::setting::ProgramId},
        .size = (u32)mNrrBufferSize,
        .type = 0,
        .hashes_offset = sizeof(nn::ro::NrrHeader),
        .num_hashes = (u32)mPluginCount
    };

    auto* hashes = reinterpret_cast<Sha256Hash*>(mNrrBuffer + sizeof(nn::ro::NrrHeader));

    size_t hashIndex = 0;
    mSortedHashes.forEach([hashes, &hashIndex](const Sha256Hash &hash) {
        hashes[hashIndex++] = hash;
    });

    // we won't need the sorted hash buffer after this point, so it can be freed
    pluginFree(mOrderedSetBuffer);
    mSortedHashes = sead::OrderedSet<Sha256Hash>(); // reset struct

    Logger::log("Created Plugin NRR.\n");
}

bool PluginLoader::registerAndLoadModules() {
    if(nn::ro::RegisterModuleInfo(&mRegistrationInfo, mNrrBuffer).isFailure() || mRegistrationInfo.state != nn::ro::RegistrationInfo::State_Registered) {
        Logger::log("Failed to register NRR!\n");
        return false;
    }

    Logger::log("NRR Registered. Loading Modules.\n");

    for (int i = 0; i < mPluginCount; ++i) {
        auto& plugin = mPlugins[i];

        if(nn::ro::LoadModule(&plugin.mModule, plugin.mFileData, plugin.mBssData, plugin.mBssSize, nn::ro::BindFlag_Now).isFailure()) {
            Logger::log("Failed to Load Module for plugin at: %s/%s\n", plugin.mFilePath, plugin.mFileName);
        }else {
            Logger::log("Loaded Module: %s\n", plugin.mFileName);
            plugin.mModuleLoaded = true;
        }
    }

    Logger::log("Modules created and Loaded.\n");

    return true;
}

bool PluginLoader::loadPlugins(const char* rootDir, bool isReload) {
    auto& inst = instance();

    // init ro
    nn::ro::Initialize();

    Logger::log("Loading nro files within %s.\n", rootDir);

    // get root dir info
    s64 nroCount = 0;
    FsHelper::DirFileEntry *fileData = FsHelper::loadFilesFromDirectory(rootDir, &nroCount, ".nro");

    Logger::log("Found %d NRO(s) from Directory.\n", nroCount);
    
    inst.preparePluginsForLoad(nroCount, fileData);

    inst.generatePluginNrr();

    if(!inst.registerAndLoadModules()) {
        Logger::log("Failed to Register/Load Plugin modules. Unable to continue.\n");
        return false;
    }

    Logger::log("Finished Loading Plugins.\n");

    inst.mIsPluginsLoaded = true;

    for (int i = 0; i < inst.mPluginCount; ++i) {
        auto& plugin = inst.mPlugins[i];
        if(!plugin.mModuleLoaded)
            continue;

        Logger::log("Running plugin_main for %s.\n", plugin.mFileName);

        LoaderCtx ctx = {
            .mRootPluginHeap = inst.mHeap,
            .mIsReload = isReload
        };
        strcpy(ctx.mLoadDir, plugin.mFilePath + 8); // strlen("sd:/smo/") required for using sead's file device system

        if(!plugin.runPluginMain(ctx)) {
            Logger::log("Plugin was not able to successfully start.\n");
            // TODO: unload individual plugin
            continue;
        }

        plugin.mHeap = ctx.mChildHeap;
    }

    return true;
}

void PluginLoader::unloadPlugins() {
    auto& inst = instance();

    for (int i = 0; i < inst.mPluginCount; ++i) {
        auto& plugin = inst.mPlugins[i];
        nn::ro::UnloadModule(&plugin.mModule);
    }

    // reset plugin heap
    inst.mHeap->freeAll();

    inst.mPluginCount = 0;
    inst.mIsPluginsLoaded = false;

    // remove all plugin events (the only one the loader uses doesn't ever run again after game init)
    EventSystem::clearAllEvents();

    // finalize ro

    nn::ro::Finalize();
}

void PluginLoader::unloadPluginByName(const char* name) {
    Logger::log("Unloading Plugin: %s\n", name);

    EventSystem::removeFromEvents(name); // removes all events registered to the plugins name (without .nro)



    // TODO: actually unload
}

void PluginLoader::unloadPluginByIdx(int index) {
    PluginData* data = getPluginData(index);
}

void PluginLoader::getPluginNames(const char** outBuffer) {
    auto& inst = instance();

    for (int i = 0; i < inst.mPluginCount; ++i) {
        outBuffer[i] = inst.mPlugins[i].mFileName;
    }
}
sead::Heap* PluginLoader::createHeap() {
    sead::ExpHeap* pluginHeap = instance().mHeap = sead::ExpHeap::create(MBTOBYTES(5), "PluginHeap",
                                                                         sead::HeapMgr::instance()->findHeapByName("SequenceHeap", 0),
                                                                         8, sead::Heap::cHeapDirection_Forward, false);
    pluginHeap->enableLock(true); // allows for plugin heap to be used across threads
    return pluginHeap;
}
sead::Heap* PluginLoader::getHeap() { return instance().mHeap; }

PluginData* PluginLoader::getPluginData(int idx) {
    auto& inst = instance();
    if(inst.mPluginCount > idx) {
        return &inst.mPlugins[idx];
    }
    return nullptr;
}

int PluginLoader::getPluginIdxByName(const char* name) {
    auto& inst = instance();

    for (int i = 0; i < inst.mPluginCount; ++i) {
        auto& pluginData = inst.mPlugins[i];
        if(strstr(pluginData.mFileName,name) != nullptr) return i;
    }

    return -1;
}
