#include "lib.hpp"
#include "imgui_backend/imgui_impl_nvn.hpp"
#include "patches.hpp"
#include "logger/Logger.hpp"

#include "nn/fs.h"
#include "nn/diag.h"

#include <al/async/FunctorV0M.hpp>
#include <basis/seadRawPrint.h>
#include <devenv/seadDebugFontMgrNvn.h>
#include <filedevice/nin/seadNinSDFileDeviceNin.h>
#include <filedevice/seadFileDeviceMgr.h>
#include <filedevice/seadPath.h>
#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <heap/seadHeapMgr.h>
#include <helpers/fsHelper.h>
#include <plugin/events/Events.h>
#include <prim/seadSafeString.h>
#include <resource/seadArchiveRes.h>
#include <resource/seadResourceMgr.h>

#include "rs/util.hpp"

#include "game/StageScene/StageScene.h"
#include "game/System/GameSystem.h"
#include "game/System/Application.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"

#include "al/util.hpp"
#include "al/fs/FileLoader.h"

#include "agl/utl.h"
#include "imgui_nvn.h"
#include "helpers/InputHelper.h"
#include "helpers/PlayerHelper.h"
#include "game/GameData/GameDataFunction.h"

#include "exception/ExceptionHandler.h"
#include "plugin/PluginLoader.h"

#define IMGUI_ENABLED true

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

void drawPluginDebugWindow() {
    ImGui::SetNextWindowPos(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);

    ImGui::Begin("Plugin Info Window");

    drawHeapInfo(PluginLoader::getHeap());

    if(!PluginLoader::isPluginsLoaded()) {
        if(ImGui::Button("Load Plugins")) {
            Logger::log("Loading Game Plugins.\n");
            PluginLoader::loadPlugins("sd:/smo/PluginData");
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
    al::Sequence *curSequence = GameSystemFunction::getGameSystem()->mCurSequence;

    ImGui::Begin("Game Debug Window");
    ImGui::SetWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    ImGui::Text("Current Sequence Name: %s", curSequence->getName().cstr());

    static bool showWindow = false;

    if (ImGui::Button("Toggle Demo Window")) {
        showWindow = !showWindow;
    }

    if (showWindow) {
        ImGui::ShowDemoWindow();
    }

    if (curSequence && al::isEqualString(curSequence->getName().cstr(), "HakoniwaSequence")) {
        auto gameSeq = (HakoniwaSequence *) curSequence;
        auto curScene = gameSeq->curScene;

        bool isInGame =
                curScene && curScene->mIsAlive && al::isEqualString(curScene->mName.cstr(), "StageScene");

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
            StageScene *stageScene = (StageScene *) gameSeq->curScene;
            PlayerActorBase *playerBase = rs::getPlayerActor(stageScene);

            if (ImGui::Button("Kill Mario")) {
                PlayerHelper::killPlayer(playerBase);
            }
        }
    }

    ImGui::End();
}

HOOK_DEFINE_TRAMPOLINE(ControlHook) {
    static void Callback(StageScene *scene) {
        Orig(scene);
    }
};

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

        thisPtr->mMountedSd = true;

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

HOOK_DEFINE_TRAMPOLINE(FileLoaderLoadArc) {
    static sead::ArchiveRes *
    Callback(al::FileLoader *thisPtr, sead::SafeString &path, const char *ext, sead::FileDevice *device) {

        // Logger::log("Path: %s\n", path.cstr());

        sead::FileDevice *sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

        if (sdFileDevice && sdFileDevice->isExistFile(path)) {

            Logger::log("Found File on SD! Path: %s\n", path.cstr());

            device = sdFileDevice;
        }

        return Orig(thisPtr, path, ext, device);
    }
};

sead::FileDevice *tryFindNewDevice(sead::SafeString &path, sead::FileDevice *orig) {
    sead::FileDevice *sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

    if (sdFileDevice && sdFileDevice->isExistFile(path))
        return sdFileDevice;

    return orig;
}

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
    Logger::instance().init(LOGGER_IP, 3080);

    // event system

    EventSystem::installAllEvents();

    GameSystemEvent::Init::addEvent(&gameSystemInitPrefix, nullptr);

    // sd mounting

    if(nn::fs::MountSdCardForDebug("sd").isSuccess()) {
        Logger::log("Mounted SD.\n");
    }

    // SD File Redirection

    RedirectFileDevice::InstallAtOffset(0x76CFE0);
    FileLoaderLoadArc::InstallAtOffset(0xA5EF64);
    CreateFileDeviceMgr::InstallAtOffset(0x76C8D4);
    FileLoaderIsExistFile::InstallAtSymbol(
            "_ZNK2al10FileLoader11isExistFileERKN4sead14SafeStringBaseIcEEPNS1_10FileDeviceE");
    FileLoaderIsExistArchive::InstallAtSymbol(
            "_ZNK2al10FileLoader14isExistArchiveERKN4sead14SafeStringBaseIcEEPNS1_10FileDeviceE");

    // Sead Debugging Overriding

    ReplaceSeadPrint::InstallAtOffset(0xB59E28);

    // General Hooks

    ControlHook::InstallAtSymbol("_ZN10StageScene7controlEv");

    // ImGui Hooks
#if IMGUI_ENABLED
    nvnImGui::InstallHooks();

    nvnImGui::addDrawFunc(drawPluginDebugWindow);

    nvnImGui::addDrawFunc([]() {
        ModEvent::ImguiDraw::RunEvents();
    });
#endif
}

extern "C" NORETURN void exl_exception_entry() {
    /* TODO: exception handling */
    EXL_ABORT(0x420);
}