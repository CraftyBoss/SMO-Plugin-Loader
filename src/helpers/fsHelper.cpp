#include "fsHelper.h"
#include "StringHelper.h"
#include "diag/assert.hpp"
#include "init.h"
#include "logger/Logger.hpp"
#include "nn/result.h"
#include <cstring>

namespace FsHelper {
    nn::Result writeFileToPath(void *buf, size_t size, const char *path) {
        nn::fs::FileHandle handle;

        if (isFileExist(path)) {
            Logger::log("Removing Previous File.\n");
            nn::fs::DeleteFile(path); // remove previous file
        }

        if (nn::fs::CreateFile(path, size)) {
            Logger::log("Failed to Create File.\n");
            return 1;
        }

        if (nn::fs::OpenFile(&handle, path, nn::fs::OpenMode_Write)) {
            Logger::log("Failed to Open File.\n");
            return 1;
        }

        if (nn::fs::WriteFile(handle, 0, buf, size, nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush))) {
            Logger::log("Failed to Write to File.\n");
            return 1;
        }

        Logger::log("Successfully wrote file to: %s!\n", path);

        nn::fs::CloseFile(handle);

        return 0;
    }

    // make sure to free buffer after usage is done
    void loadFileFromPath(LoadData &loadData) {

        nn::fs::FileHandle handle;

        EXL_ASSERT(FsHelper::isFileExist(loadData.path), "Failed to Find File!\nPath: %s", loadData.path);

        R_ABORT_UNLESS(nn::fs::OpenFile(&handle, loadData.path, nn::fs::OpenMode_Read))

        long size = 0;
        nn::fs::GetFileSize(&size, handle);
        loadData.buffer = nn::init::GetAllocator()->Allocate(size);
        loadData.bufSize = size;

        EXL_ASSERT(loadData.buffer, "Failed to Allocate Buffer! File Size: %ld", size);

        R_ABORT_UNLESS(nn::fs::ReadFile(handle, 0, loadData.buffer, size))

        nn::fs::CloseFile(handle);
    }

    // local overload for directory file entries
    void loadFileFromPath(DirFileEntry &loadData) {

        nn::fs::FileHandle handle;

        EXL_ASSERT(FsHelper::isFileExist(loadData.fullPath), "Failed to Find File!\nPath: %s", loadData.fullPath);

        R_ABORT_UNLESS(nn::fs::OpenFile(&handle, loadData.fullPath, nn::fs::OpenMode_Read))

        long size = 0;
        nn::fs::GetFileSize(&size, handle);
        loadData.fileBuffer = (u8*)nn::init::GetAllocator()->Allocate(size);
        loadData.bufSize = size;

        EXL_ASSERT(loadData.fileBuffer, "Failed to Allocate Buffer! File Size: %ld", size);

        R_ABORT_UNLESS(nn::fs::ReadFile(handle, 0, loadData.fileBuffer, size))

        nn::fs::CloseFile(handle);
    }

    long getFileSize(const char *path) {
        nn::fs::FileHandle handle;
        long result = -1;

        nn::Result openResult = nn::fs::OpenFile(&handle, path, nn::fs::OpenMode::OpenMode_Read);

        if (openResult.isSuccess()) {
            nn::fs::GetFileSize(&result, handle);
            nn::fs::CloseFile(handle);
        }

        return result;
    }

    const char* getFileName(const char* path) {
//        if(path == nullptr)
//            return nullptr;
//
//        const char* pFileName = path;
//        for(const char* pCur = path; *pCur != '\0'; pCur++)
//        {
//            if( *pCur == '/' || *pCur == '\\' )
//                pFileName = pCur+1;
//        }

        return strrchr(path, '/') + 1;
    }

    bool isFileExist(const char *path) {
        nn::fs::DirectoryEntryType type;
        nn::Result result = nn::fs::GetEntryType(&type, path);

        return type == nn::fs::DirectoryEntryType_File;
    }

    void freeFile(LoadData &data) {
        if(data.buffer)
            nn::init::GetAllocator()->Free(data.buffer);
    }

    void freeFile(DirFileEntry &entry) {
        if(entry.fileBuffer)
            nn::init::GetAllocator()->Free(entry.fileBuffer);
    }

    void freeEntries(DirFileEntry* entries, size_t count) {
        if(count > 0) {
            for (int i = 0; i < count; ++i) {
                freeFile(entries[i]);
            }
        }
        nn::init::GetAllocator()->Free(entries);
    }

    DirFileEntry* loadFilesFromDirectory(const char* dir, s64 *outFileCount, const char *ext) {
        nn::fs::DirectoryEntryType type;
        nn::Result result = nn::fs::GetEntryType(&type, dir);
        *outFileCount = 0;

        if(result.isFailure() || type != nn::fs::DirectoryEntryType_Directory) {
            return nullptr;
        }

        nn::fs::DirectoryHandle handle{};
        result = nn::fs::OpenDirectory(&handle, dir, nn::fs::OpenDirectoryMode_All);
        if(result.isFailure()) {
            Logger::log("Failed to open directory!\n");
            return nullptr;
        }

        s64 fileCount = 0;
        result = nn::fs::GetDirectoryEntryCount(&fileCount, handle);
        if(result.isFailure()) {
            Logger::log("Failed to get entry count!\n");
            return nullptr;
        }

        auto* fileEntries = (DirFileEntry*)alloca(sizeof(DirFileEntry)*fileCount);
        auto* entries = (nn::fs::DirectoryEntry*)alloca(sizeof(nn::fs::DirectoryEntry)*fileCount);

        s64 entryCount = 0;
        result = nn::fs::ReadDirectory(&entryCount, entries, handle, fileCount);
        if(result.isFailure()) {
            Logger::log("Failed to Read Directories!\n");
            return nullptr;
        }

        int loadedFileCount = 0;

        for (int i = 0; i < entryCount; ++i) {
            nn::fs::DirectoryEntry& entry = entries[i];
            DirFileEntry& fileEntry = fileEntries[loadedFileCount];

            Logger::log("File Name: %s Size: %d Type: %x\n", entry.m_Name, entry.m_FileSize, entry.m_Type);

            size_t pathBufSize = strlen(dir)+strlen(entry.m_Name)+2;
            char dirPathBuffer[pathBufSize];
            snprintf(dirPathBuffer, pathBufSize, "%s/%s", dir, entry.m_Name);

            nn::fs::DirectoryEntryType entryType;
            result = nn::fs::GetEntryType(&entryType, dirPathBuffer);

            if(result.isFailure()) {
                Logger::log("Failed to get Entry Type.\n");
                continue;
            }

            switch (entryType) {
            case nn::fs::DirectoryEntryType_Directory: {
                nn::Result subResult = {};

                nn::fs::DirectoryHandle subdirHandle{};
                subResult = nn::fs::OpenDirectory(&subdirHandle, dirPathBuffer, nn::fs::OpenDirectoryMode_File);
                if(subResult.isFailure()) {
                    Logger::log("Failed to open directory!\n");
                    continue;
                }

                s64 subdirFileCount = 0;
                subResult = nn::fs::GetDirectoryEntryCount(&subdirFileCount, subdirHandle);
                if(subResult.isFailure()) {
                    Logger::log("Failed to get entry count!\n");
                    continue;
                }

                auto* subdirEntries = (nn::fs::DirectoryEntry*)alloca(sizeof(nn::fs::DirectoryEntry)*subdirFileCount);

                s64 subdirEntryCount = 0;
                subResult = nn::fs::ReadDirectory(&subdirEntryCount, subdirEntries, subdirHandle, subdirFileCount);
                if(subResult.isFailure()) {
                    Logger::log("Failed to Read Directories!\n");
                    continue;
                }

                for (int j = 0; j < subdirFileCount; ++j) {
                    nn::fs::DirectoryEntry& subDirEntry = subdirEntries[j];
                    Logger::log("Sub Directory File Name: %s Size: %d Type: %x\n", subDirEntry.m_Name, subDirEntry.m_FileSize, subDirEntry.m_Type);

                    if(ext && !StringHelper::isEndWithString(subDirEntry.m_Name, ext)) {
                        Logger::log("File extension does not match! Ext: %s\n", ext);
                        continue;
                    }

                    sprintf(fileEntry.fullPath, "%s/%s", dirPathBuffer, subDirEntry.m_Name);
                    fileEntry.bufSize = subDirEntry.m_FileSize;

                    Logger::log("Full Path: %s\n", fileEntry.fullPath);

                    loadFileFromPath(fileEntry);
                    loadedFileCount++;
                }

                break;
            }
            case nn::fs::DirectoryEntryType_File: {

                if(ext && !StringHelper::isEndWithString(entry.m_Name, ext)) {
                    Logger::log("File extension does not match! Ext: %s\n", ext);
                    continue;
                }

                strncpy(fileEntry.fullPath, dirPathBuffer, sizeof(fileEntry.fullPath));
                fileEntry.bufSize = entry.m_FileSize;

                loadFileFromPath(fileEntry);
                loadedFileCount++;
                break;
            }
            default:
                Logger::log("Unknown Open Directory Mode!\n");
                break;
            }


        }

        auto* loadedEntries = (DirFileEntry*)nn::init::GetAllocator()->Allocate(sizeof(DirFileEntry) * loadedFileCount);
        memcpy(loadedEntries, fileEntries, sizeof(DirFileEntry) * loadedFileCount);

        *outFileCount = loadedFileCount;
        return loadedEntries;
    }
}