#pragma once

#include "PluginData.h"
#include "helpers.h"

#include <set>
#include <cstring>

#include "nn/crypto.h"
#include "nn/ro.h"
#include "sha256.h"
#include <container/seadOrderedSet.h>
#include <heap/seadExpHeap.h>

// A decent amount of this implementation references skyline
// (https://github.com/skyline-dev/skyline/blob/master/source/skyline)

class PluginLoader {

    PluginData* mPlugins = {};
    size_t mPluginCount = {};

    sead::ExpHeap* mHeap;

    sead::OrderedSet<Sha256Hash> mSortedHashes;
    void *mOrderedSetBuffer;

    u8* mNrrBuffer = {};
    size_t mNrrBufferSize = {};

    nn::ro::RegistrationInfo mRegistrationInfo = {};

    bool mIsPluginsLoaded = false;

    bool createPluginData(PluginData& data, const FsHelper::DirFileEntry& entry);

    void preparePluginsForLoad(s32 nroCount, FsHelper::DirFileEntry *fileData);

    void generatePluginNrr();

    bool registerAndLoadModules();

public:

    static PluginLoader& instance();

    static bool loadPlugins(const char* rootDir);

    static void unloadPlugins();

    static void unloadPluginByName(const char* name);

    static void unloadPluginByIdx(int index);

    static sead::Heap* createHeap();

    static sead::Heap* getHeap();

    static bool isPluginsLoaded() { return instance().mIsPluginsLoaded; }

    static size_t getPluginCount() { return instance().mPluginCount; }

    static void getPluginNames(const char** outBuffer);

    static PluginData* getPluginData(int idx);

    static int getPluginIdxByName(const char *name);

};
