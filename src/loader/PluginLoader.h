#pragma once

#include "helpers.h"
#include "nn/crypto.h"
#include "nn/ro.h"
#include <cstring>
#include <set>

#include <container/seadOrderedSet.h>
#include <heap/seadExpHeap.h>

// A decent amount of this implementation references skyline
// (https://github.com/skyline-dev/skyline/blob/master/source/skyline)

class PluginLoader {

    // pretty much identical to skylines impl
    struct Sha256Hash {
        u8 data[0x20] = {};

        bool operator==(const Sha256Hash &rs) const { return memcmp(this->data, rs.data, sizeof(this->data)) == 0; }
        bool operator!=(const Sha256Hash &rs) const { return !operator==(rs); }

        bool operator<(const Sha256Hash &rs) const { return memcmp(this->data, rs.data, sizeof(this->data)) < 0; }
        bool operator>(const Sha256Hash &rs) const { return memcmp(this->data, rs.data, sizeof(this->data)) > 0; }

        void print() {
            for (u8 i : data) { Logger::log("%02X", i); }
            Logger::log("\n");
        }
    };

    struct PluginData {
        char path[0x40] = {};
        Sha256Hash hash = {};

        u8* fileData = nullptr;
        size_t fileSize = 0;

        nn::ro::Module module = {};

        u8 *bssData = nullptr;
        size_t bssSize = 0;
    };

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

    static sead::Heap* createHeap();

    static sead::Heap* getHeap();

    static bool isPluginsLoaded() { return instance().mIsPluginsLoaded; }

    static size_t getPluginCount() { return instance().mPluginCount; }

    static void getPluginNames(const char** outBuffer);

};
