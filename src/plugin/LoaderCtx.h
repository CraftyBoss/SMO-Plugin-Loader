#pragma once
#include <heap/seadHeap.h>

// struct passed to each plugin to provide any needed context from the plugin loader.
// also used to return data to the loader.
struct LoaderCtx {
    sead::Heap *mRootPluginHeap;
    sead::Heap* mChildHeap; // heap created by plugin
};