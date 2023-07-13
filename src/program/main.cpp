#include "lib.hpp"
#include "imgui_backend/imgui_impl_nvn.hpp"
#include "patches.hpp"
#include "logger/Logger.hpp"
#include "imgui_nvn.h"
#include "helpers/PlayerHelper.h"

#include "exception/ExceptionHandler.h"
#include "plugin/PluginLoader.h"

#include "nn/fs.h"

#include <basis/seadRawPrint.h>
#include <filedevice/nin/seadNinSDFileDeviceNin.h>
#include <filedevice/seadFileDeviceMgr.h>
#include <filedevice/seadPath.h>
#include <gfx/seadViewport.h>
#include <plugin/events/Events.h>
#include <prim/seadSafeString.h>
#include <resource/seadArchiveRes.h>
#include <resource/seadResourceMgr.h>

#include "rs/util.hpp"

#include "game/StageScene/StageScene.h"
#include "game/System/GameSystem.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/GameData/GameDataFunction.h"

#include <al/Library/File/FIleEntry.h>
#include <al/Library/File/FileLoader.h>
#include <nifm.h>

#define IMGUI_ENABLED true

//#define DEBUG

char socketPool[0x600000 + 0x20000] __attribute__((aligned(0x1000)));

void initSocket() {
    nn::nifm::Initialize();

    nn::socket::Config config;

    config.pool = socketPool;
    config.poolSize = 0x600000;
    config.allocPoolSize = 0x20000;
    config.concurLimit = 0xE;

    nn::socket::Initialize(config);

    nn::nifm::SubmitNetworkRequest();

    while (nn::nifm::IsNetworkRequestOnHold()) {}
}

void drawSizeInfo(float curSize, float maxSize, const char* name) {

    // convert size to megabytes for readability
    curSize = BYTESTOMB(curSize);
    maxSize = BYTESTOMB(maxSize);

    float progress = curSize / maxSize;

    char buf[0x60];
    sprintf(buf, "%s: %.3fmb/%.3fmb", name, curSize, maxSize);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(0,200,0,255));
    ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.f), buf);
    ImGui::PopStyleColor();
}

void drawSizeInfo(float curSize, float maxSize) {

    // convert size to megabytes for readability
    curSize = BYTESTOMB(curSize);
    maxSize = BYTESTOMB(maxSize);

    float progress = curSize / maxSize;

    char buf[0x60];
    sprintf(buf, "%.3fmb/%.3fmb", curSize, maxSize);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(0,200,0,255));
    ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.f), buf);
    ImGui::PopStyleColor();
}

void drawHeapInfo(sead::Heap *heap) {
    if(!heap)
        return;

    u64 size = heap->getSize();
    u64 freeSize = heap->getFreeSize();

    drawSizeInfo(size - freeSize,  size, heap->getName().cstr());
}

void drawHeapInfoRecursive(sead::Heap *heap) {
    if(!heap)
        return;

    char buf[0x60];
    sprintf(buf, "%s Info", heap->getName().cstr());
    if(ImGui::TreeNode(buf)) {
        u64 size = heap->getSize();
        u64 freeSize = heap->getFreeSize();
        drawSizeInfo(size - freeSize,  size);

        if(ImGui::TreeNode("Children")) {
            for (auto &child : heap->mChildren) {
                drawHeapInfoRecursive(&child);
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

static bool isLogFileLoad = false;

void drawPluginDebugWindow() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);

    ImGui::Begin("Plugin Info Window");

    drawHeapInfo(PluginLoader::getHeap());

    if(ImGui::Button("Toggle File Load Logging")) {
        isLogFileLoad = !isLogFileLoad;
    }
    ImGui::SameLine();
    ImGui::Text("%s", isLogFileLoad ? "Enabled" : "Disabled");

    if(!PluginLoader::isPluginsLoaded()) {
        if(ImGui::Button("Load Plugins")) {
            Logger::log("Loading Game Plugins.\n");
            PluginLoader::loadPlugins("sd:/smo/PluginData", true);
        }
        ImGui::End();
        return;
    }

    if(ImGui::Button("Unload Plugins")) {
        Logger::log("Unloading Game Plugins.\n");
        PluginLoader::unloadPlugins();
    }

    size_t pluginCount = PluginLoader::getPluginCount();

    ImGui::Text("Plugin Count: %zu", pluginCount);

    const char* pluginNames[pluginCount];
    PluginLoader::getPluginNames(pluginNames);

    if(ImGui::TreeNode("Plugin Info")) {
        for (int i = 0; i < pluginCount; ++i) {
            char idBuf[0x40] = {};
            sprintf(idBuf, "Plugin: %s", pluginNames[i]);
            if(ImGui::TreeNode(idBuf)) {
                PluginData* plugin = PluginLoader::getPluginData(i);
                ImGui::Text("Is Plugin Loaded?: %s", BTOC(plugin->mModuleLoaded));
                if(!plugin->mModuleLoaded) {
                    continue;
                }
                char hashStr[65] = {};
                plugin->mPluginHash.sprint(hashStr);
                ImGui::Text("Plugin Hash: %s", hashStr);

                drawHeapInfo(plugin->mHeap);

                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();

}

void drawDebugWindow() {
    HakoniwaSequence *gameSeq = (HakoniwaSequence *) GameSystemFunction::getGameSystem()->mCurSequence;

    ImGui::Begin("Game Debug Window");
    ImGui::SetWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    ImGui::Text("Current Sequence Name: %s", gameSeq->getName().cstr());

    static bool showWindow = false;

    if (ImGui::Button("Toggle Demo Window")) {
        showWindow = !showWindow;
    }

    if (showWindow) {
        ImGui::ShowDemoWindow();
    }

    auto curScene = gameSeq->mStageScene;

    bool isInGame =
        curScene && curScene->mIsAlive;

    if (ImGui::CollapsingHeader("World List")) {
        for (auto &entry: gameSeq->mGameDataHolder.mData->mWorldList->mWorldList) {
            if (ImGui::TreeNode(entry.mMainStageName)) {

                if (isInGame) {
                    if (ImGui::Button("Warp to World")) {
                        PlayerHelper::warpPlayer(entry.mMainStageName, gameSeq->mGameDataHolder);
                    }
                }

                ImGui::BulletText("Clear Main Scenario: %d", entry.mClearMainScenario);
                ImGui::BulletText("Ending Scenario: %d", entry.mEndingScenario);
                ImGui::BulletText("Moon Rock Scenario: %d", entry.mMoonRockScenario);

                if (ImGui::TreeNode("Main Quest Infos")) {
                    for (int i = 0; i < entry.mQuestInfoCount; ++i) {
                        ImGui::BulletText("Quest %d Scenario: %d", i, entry.mMainQuestIndexes[i]);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::CollapsingHeader("Database Entries")) {
                    for (auto &dbEntry: entry.mStageNames) {
                        if (ImGui::TreeNode(dbEntry.mStageName.cstr())) {
                            ImGui::BulletText("Stage Category: %s", dbEntry.mStageCategory.cstr());
                            ImGui::BulletText("Stage Use Scenario: %d", dbEntry.mUseScenario);

                            if (isInGame) {
                                ImGui::Bullet();
                                if (ImGui::SmallButton("Warp to Stage")) {
                                    PlayerHelper::warpPlayer(dbEntry.mStageName.cstr(),
                                                             gameSeq->mGameDataHolder);
                                }
                            }

                            ImGui::TreePop();
                        }
                    }
                }

                ImGui::TreePop();
            }
        }
    }

    if (isInGame) {
        StageScene *stageScene = gameSeq->mStageScene;
        PlayerActorBase *playerBase = rs::getPlayerActor(stageScene);

        if (ImGui::Button("Kill Mario")) {
            PlayerHelper::killPlayer(playerBase);
        }
    }

    ImGui::End();
}

HOOK_DEFINE_REPLACE(ReplaceSeadPrint) {
    static void Callback(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Logger::log(format, args);
        va_end(args);
    }
};

HOOK_DEFINE_TRAMPOLINE(CreateFileDeviceMgr) {
    static void Callback(sead::FileDeviceMgr *thisPtr) {

        Orig(thisPtr);

        thisPtr->mMountedSd = true; // we mount sd card during module init

        sead::NinSDFileDevice *sdFileDevice = new sead::NinSDFileDevice();

        thisPtr->mount(sdFileDevice);

    }
};

HOOK_DEFINE_TRAMPOLINE(RedirectFileDevice) {
    static sead::FileDevice *
    Callback(sead::FileDeviceMgr *thisPtr, sead::SafeString &path, sead::BufferedSafeString *pathNoDrive) {
        sead::FixedSafeString<32> driveName;
        sead::FileDevice *device;

        // Logger::log("Path: %s\n", path.cstr());

        if (!sead::Path::getDriveName(&driveName, path)) {

            device = thisPtr->findDevice("sd");

            if (!(device && device->isExistFile(path))) {

                device = thisPtr->getDefaultFileDevice();

                if (!device) {
                    Logger::log("drive name not found and default file device is null\n");
                    return nullptr;
                }

            } else {
                Logger::log("Found File on SD! Path: %s\n", path.cstr());
            }

        } else
            device = thisPtr->findDevice(driveName);

        if (!device)
            return nullptr;

        if (pathNoDrive != nullptr)
            sead::Path::getPathExceptDrive(pathNoDrive, path);

        return device;
    }
};

sead::FileDevice *tryFindNewDevice(sead::SafeString &path, sead::FileDevice *orig) {
    sead::FileDevice *sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

    if (sdFileDevice && sdFileDevice->isExistFile(path))
        return sdFileDevice;
    return orig;
}

HOOK_DEFINE_TRAMPOLINE(FileLoaderLoadArc) {
    static sead::ArchiveRes *
    Callback(al::FileLoader *thisPtr, sead::SafeString &path, const char *ext, sead::FileDevice *device) {
        return Orig(thisPtr, path, ext, tryFindNewDevice(path, device));
    }
};

HOOK_DEFINE_TRAMPOLINE(FileLoaderIsExistFile) {
    static bool Callback(al::FileLoader *thisPtr, sead::SafeString &path, sead::FileDevice *device) {
        return Orig(thisPtr, path, tryFindNewDevice(path, device));
    }
};

HOOK_DEFINE_TRAMPOLINE(FileLoaderIsExistArchive) {
    static bool Callback(al::FileLoader *thisPtr, sead::SafeString &path, sead::FileDevice *device) {
        return Orig(thisPtr, path, tryFindNewDevice(path, device));
    }
};

HOOK_DEFINE_TRAMPOLINE(FileLoaderThreadLoadFileHook) {
    static void Callback(al::FileLoaderThread *thisPtr, al::FileEntryBase* fileEntry) {

        if(isLogFileLoad)
            Logger::log("Loading File: %s\n", fileEntry->mFileName.cstr());

        Orig(thisPtr, fileEntry);
    }
};

HOOK_DEFINE_TRAMPOLINE(CheckPlayerDamageHook) {
    static void Callback(GameDataHolderWriter writer) {
        // TODO: add an argument to OnPlayerDamage for if the player is dead
        Orig(writer);
        ModEvent::OnPlayerDamage::RunEvents();
    }
};

bool gameSystemInitPrefix(GameSystem*){
    PluginLoader::createHeap();

    Logger::log("Loading Game Plugins.\n");

    handler::tryCatch([]() {
        PluginLoader::loadPlugins("sd:/smo/PluginData");
    }, [](handler::ExceptionInfo& info) {
        Logger::log("Exception caught while loading plugins. Unable to continue loading.\n");
        handler::printStackTrace(info, 1, true);
        return true;
    });

    return true;
}

extern "C" void exl_main(void *x0, void *x1) {
    /* Setup hooking enviroment. */
    exl::hook::Initialize();

    handler::installExceptionHandler([](handler::ExceptionInfo& info) {
        handler::printCrashReport(info);
        return false;
    });

    runCodePatches();

    initSocket();

#if ISEMU
    Logger::setLogType(LoggerType::Emulator);
#elseif DEBUG
    Logger::setLogType(LoggerType::Network);
#else
    Logger::setLogType(LoggerType::ImGui);
#endif

    // event system

    EventSystem::installAllEvents();

    CheckPlayerDamageHook::InstallAtSymbol("_ZN16GameDataFunction12damagePlayerE20GameDataHolderWriter");

    GameSystemEvent::Init::addEvent(&gameSystemInitPrefix, nullptr);

    // sd mounting

    if(nn::fs::MountSdCardForDebug("sd").isSuccess()) {
        Logger::log("Mounted SD.\n");
    }

    // SD File Redirection

    RedirectFileDevice::InstallAtSymbol("_ZNK4sead13FileDeviceMgr18findDeviceFromPathERKNS_14SafeStringBaseIcEEPNS_22BufferedSafeStringBaseIcEE");
    FileLoaderLoadArc::InstallAtSymbol("_ZN2al10FileLoader16loadArchiveLocalERKN4sead14SafeStringBaseIcEEPKcPNS1_10FileDeviceE");
    CreateFileDeviceMgr::InstallAtSymbol("_ZN4sead13FileDeviceMgrC2Ev");
    FileLoaderIsExistFile::InstallAtSymbol("_ZNK2al10FileLoader11isExistFileERKN4sead14SafeStringBaseIcEEPNS1_10FileDeviceE");
    FileLoaderIsExistArchive::InstallAtSymbol("_ZNK2al10FileLoader14isExistArchiveERKN4sead14SafeStringBaseIcEEPNS1_10FileDeviceE");

    // Sead Debugging Overriding

    ReplaceSeadPrint::InstallAtSymbol("_ZN4sead6system5PrintEPKcz");

    // File Load Logging

    FileLoaderThreadLoadFileHook::InstallAtSymbol("_ZN2al16FileLoaderThread15requestLoadFileEPNS_13FileEntryBaseE");

    // ImGui Hooks
#if IMGUI_ENABLED
    nvnImGui::InstallHooks();

    nvnImGui::addDrawFunc(drawPluginDebugWindow);

    // TODO: add plugin window drawing visibility toggle
    nvnImGui::addDrawFunc([]() {
        ModEvent::ImguiDraw::RunEvents();
    });
#endif
}

extern "C" NORETURN void exl_exception_entry() {
    /* TODO: exception handling */
    EXL_ABORT(0x420);
}