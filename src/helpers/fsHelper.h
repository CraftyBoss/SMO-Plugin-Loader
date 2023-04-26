#pragma once

#include "nn/fs.h"
#include "nn/result.h"

namespace FsHelper {

    struct LoadData {
        const char *path;
        void *buffer;
        long bufSize;
    };

    struct DirFileEntry {
        char fullPath[0x40];
        u8 *fileBuffer;
        long bufSize;
    };

    nn::Result writeFileToPath(void *buf, size_t size, const char *path);

    void loadFileFromPath(LoadData &loadData);

    DirFileEntry* loadFilesFromDirectory(const char* dir, s64 *outFileCount, const char *ext = nullptr);

    void freeFile(LoadData &data);

    void freeFile(DirFileEntry &entry);

    void freeEntries(DirFileEntry* entries, size_t count = 0);

    long getFileSize(const char *path);

    const char* getFileName(char *path);

    bool isFileExist(const char *path);
}
