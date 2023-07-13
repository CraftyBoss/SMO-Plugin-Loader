#pragma once
#include <heap/seadHeap.h>

// struct passed to each plugin to provide any needed context from the plugin loader.
// also used to return data to the loader.
struct LoaderCtx {
    sead::Heap *mRootPluginHeap = nullptr;
    sead::Heap* mChildHeap = nullptr; // heap created by plugin
    char mLoadDir[0x40] = {};
    bool mIsReload = false;
};