#include "lib.hpp"
#include <heap/seadHeapMgr.h>
#include <loader/PluginLoader.h>

#include "nn/init.h"

PluginLoader& PluginLoader::instance() {
    static PluginLoader sInstance;
    return sInstance;
}

EXPORT_SYM void* pluginAlloc(size_t size, s32 alignment = 8) {
    return nn::init::GetAllocator()->Allocate(ALIGN_UP(size, alignment));
//    return PluginLoader::getHeap()->alloc(size, alignment);
}

EXPORT_SYM void pluginFree(void* ptr) {
    nn::init::GetAllocator()->Free(ptr);
//    PluginLoader::getHeap()->free(ptr);
}

EXPORT_SYM void* pluginRealloc(void* ptr, size_t size) {
    return nn::init::GetAllocator()->Reallocate(ptr, size);
//    return PluginLoader::getHeap()->tryRealloc(ptr, size, 8);
}

bool PluginLoader::createPluginData(PluginData& data, const FsHelper::DirFileEntry& entry) {

    Logger::log("Size: %u\n", entry.bufSize);

    data.fileSize = entry.bufSize;
    data.fileData = (u8*)pluginAlloc(data.fileSize, 0x1000); // must be aligned
    memcpy(data.fileData, entry.fileBuffer, data.fileSize);
    memcpy(data.path, entry.fullPath, sizeof(data.path));

    if(nn::ro::GetBufferSize(&data.bssSize, data.fileData).isFailure()) {
        Logger::log("Failed to get NRO Buffer Size! NRO may not be valid!\n");
        pluginFree(data.fileData);
        return false;
    }

    Logger::log("NRO Buffer size: %d\n", data.bssSize);

    data.bssData = (u8*)pluginAlloc(data.bssSize, 0x1000);

    // hash header for NRR registration later
    auto* nroHeader = (nn::ro::NroHeader*)data.fileData;
    nn::crypto::GenerateSha256Hash(&data.hash, sizeof(data.hash), nroHeader, nroHeader->size);
    
    Logger::log("NRO Hash: ");
    data.hash.print();

    // register hash to set

    if(mSortedHashes.find(data.hash) != nullptr) {
        Logger::log("Plugin has already been registered! Skipping.\n");
        pluginFree(data.fileData);
        return false;
    }

    mSortedHashes.insert(data.hash);

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
    mSortedHashes.forEach([hashes, hashIndex](const Sha256Hash &hash) {
        hashes[hashIndex] = hash;
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

        if(nn::ro::LoadModule(&plugin.module, plugin.fileData, plugin.bssData, plugin.bssSize, nn::ro::BindFlag_Now).isFailure()) {
            Logger::log("Failed to Load Module for plugin at: %s\n", plugin.path);
        }else {
            Logger::log("Loaded Module: %s\n", FsHelper::getFileName(plugin.path));
        }
    }

    Logger::log("Modules created and Loaded.\n");

    return true;
}

bool PluginLoader::loadPlugins(const char* rootDir) {
    auto& inst = instance();

    // init ro
    nn::ro::Initialize();

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

    return true;

}

void PluginLoader::unloadPlugins() {
    auto& inst = instance();

    for (int i = 0; i < inst.mPluginCount; ++i) {
        auto& plugin = inst.mPlugins[i];

        nn::ro::UnloadModule(&plugin.module);

        pluginFree(plugin.fileData);
        pluginFree(plugin.bssData);

    }

    // free buffers

    pluginFree(inst.mPlugins);
    pluginFree(inst.mNrrBuffer);

    inst.mPluginCount = 0;
    inst.mIsPluginsLoaded = false;

    // finalize ro

    nn::ro::Finalize();
}

void PluginLoader::getPluginNames(const char** outBuffer) {
    auto& inst = instance();

    for (int i = 0; i < inst.mPluginCount; ++i) {
        outBuffer[i] = FsHelper::getFileName(inst.mPlugins[i].path);
    }
}

sead::Heap* PluginLoader::createHeap() {
    return instance().mHeap = sead::ExpHeap::create(MBTOBYTES(5), "PluginHeap", sead::HeapMgr::instance()->findHeapByName("SequenceHeap", 0), 8, sead::Heap::cHeapDirection_Forward, false);
}

sead::Heap* PluginLoader::getHeap() { return instance().mHeap; }