#pragma once

#include <prim/seadSafeString.h>
#include <thread/seadMessageQueue.h>

namespace al {
    class FileEntryBase {
    public:

        enum LoadState
        {
            LoadClear = 0x0,
            LoadRequested = 0x1,
            LoadMessageDone = 0x2,
            LoadDone = 0x3,
        };

        FileEntryBase(void);

        virtual void load() = 0;

        void setFileName(sead::SafeString const&);
        void getFileName(void);
        void sendMessageDone(void);
        void waitLoadDone(void);
        void clear(void);
        void setLoadStateRequested(void);

        sead::FixedSafeString<0x40> mFileName;
        LoadState mLoadState;
        sead::MessageQueue mMessageQueue;
    };

    static_assert(sizeof(FileEntryBase) == 0xB8, "FileEntryBase Size");
}