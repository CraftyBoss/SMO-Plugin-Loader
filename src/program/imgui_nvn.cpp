#include "imgui_nvn.h"
#include "helpers/InputHelper.h"
#include "imgui_backend/imgui_impl_nvn.hpp"
#include "init.h"
#include "lib.hpp"
#include "logger/Logger.hpp"
#include "nvn_CppFuncPtrImpl.h"

nvn::Device *nvnDevice;
nvn::Queue *nvnQueue;
nvn::CommandBuffer *nvnCmdBuf;

nvn::DeviceGetProcAddressFunc tempGetProcAddressFuncPtr;

nvn::CommandBufferInitializeFunc tempBufferInitFuncPtr;
nvn::DeviceInitializeFunc tempDeviceInitFuncPtr;
nvn::QueueInitializeFunc tempQueueInitFuncPtr;
nvn::QueuePresentTextureFunc tempPresentTexFunc;

nvn::WindowSetCropFunc tempSetCropFunc;

bool hasInitImGui = false;

namespace nvnImGui {
    ProcDrawFunc drawQueue[100];
    size_t drawQueueCount = 0;
}

#define IMGUI_USEEXAMPLE_DRAW false

void setCrop(nvn::Window *window, int x, int y, int w, int h) {
    tempSetCropFunc(window, x, y, w, h);

    if (hasInitImGui) {

        ImVec2 &dispSize = ImGui::GetIO().DisplaySize;
        ImVec2 windowSize = ImVec2(w - x, h - y);

        if (dispSize.x != windowSize.x && dispSize.y != windowSize.y) {

            // might be a dumb way to detect if docked
            bool isDockedMode = !(windowSize.x == 1280 && windowSize.y == 720);

            dispSize = windowSize;
            ImguiNvnBackend::updateProjection(windowSize);
            ImguiNvnBackend::updateScale(isDockedMode);

        }
    }
}

void presentTexture(nvn::Queue *queue, nvn::Window *window, int texIndex) {

    if (hasInitImGui)
        nvnImGui::procDraw();

    tempPresentTexFunc(queue, window, texIndex);
}

NVNboolean deviceInit(nvn::Device *device, const nvn::DeviceBuilder *builder) {
    NVNboolean result = tempDeviceInitFuncPtr(device, builder);
    nvnDevice = device;
    nvn::nvnLoadCPPProcs(nvnDevice, tempGetProcAddressFuncPtr);
    return result;
}

NVNboolean queueInit(nvn::Queue *queue, const nvn::QueueBuilder *builder) {
    NVNboolean result = tempQueueInitFuncPtr(queue, builder);
    nvnQueue = queue;
    return result;
}

NVNboolean cmdBufInit(nvn::CommandBuffer *buffer, nvn::Device *device) {
    NVNboolean result = tempBufferInitFuncPtr(buffer, device);
    nvnCmdBuf = buffer;

    if (!hasInitImGui) {
        hasInitImGui = nvnImGui::InitImGui();
    }

    return result;
}

nvn::GenericFuncPtrFunc getProc(nvn::Device *device, const char *procName) {

    nvn::GenericFuncPtrFunc ptr = tempGetProcAddressFuncPtr(nvnDevice, procName);

    if (strcmp(procName, "nvnQueueInitialize") == 0) {
        tempQueueInitFuncPtr = (nvn::QueueInitializeFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &queueInit;
    } else if (strcmp(procName, "nvnCommandBufferInitialize") == 0) {
        tempBufferInitFuncPtr = (nvn::CommandBufferInitializeFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &cmdBufInit;
    } else if (strcmp(procName, "nvnWindowSetCrop") == 0) {
        tempSetCropFunc = (nvn::WindowSetCropFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &setCrop;
    } else if (strcmp(procName, "nvnQueuePresentTexture") == 0) {
        tempPresentTexFunc = (nvn::QueuePresentTextureFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &presentTexture;
    }

    return ptr;
}

void disableButtons(nn::hid::NpadBaseState *state) {
    if (!InputHelper::isReadInputs() && InputHelper::isInputToggled()) {
        // clear out the data within the state (except for the sampling number and attributes)
        state->mButtons = nn::hid::NpadButtonSet();
        state->mAnalogStickL = nn::hid::AnalogStickState();
        state->mAnalogStickR = nn::hid::AnalogStickState();
    }
}

HOOK_DEFINE_TRAMPOLINE(DisableFullKeyState) {
    static int Callback(int *unkInt, nn::hid::NpadFullKeyState *state, int count, uint const &port) {
        int result = Orig(unkInt, state, count, port);
        disableButtons(state);
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(DisableHandheldState) {
    static int Callback(int *unkInt, nn::hid::NpadHandheldState *state, int count, uint const &port) {
        int result = Orig(unkInt, state, count, port);
        disableButtons(state);
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(DisableJoyDualState) {
    static int Callback(int *unkInt, nn::hid::NpadJoyDualState *state, int count, uint const &port) {
        int result = Orig(unkInt, state, count, port);
        disableButtons(state);
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(DisableJoyLeftState) {
    static int Callback(int *unkInt, nn::hid::NpadJoyLeftState *state, int count, uint const &port) {
        int result = Orig(unkInt, state, count, port);
        disableButtons(state);
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(DisableJoyRightState) {
    static int Callback(int *unkInt, nn::hid::NpadJoyRightState *state, int count, uint const &port) {
        int result = Orig(unkInt, state, count, port);
        disableButtons(state);
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(NvnBootstrapHook) {
    static void *Callback(const char *funcName) {

        void *result = Orig(funcName);

        if (strcmp(funcName, "nvnDeviceInitialize") == 0) {
            tempDeviceInitFuncPtr = (nvn::DeviceInitializeFunc) result;
            return (void *) &deviceInit;
        }
        if (strcmp(funcName, "nvnDeviceGetProcAddress") == 0) {
            tempGetProcAddressFuncPtr = (nvn::DeviceGetProcAddressFunc) result;
            return (void *) &getProc;
        }

        return result;
    }
};

void nvnImGui::addDrawFunc(ProcDrawFunc func) {
    drawQueue[drawQueueCount++] = func;
}

void nvnImGui::procDraw() {


    ImguiNvnBackend::newFrame();
    ImGui::NewFrame();


    static bool isEnabled = false;
    if(InputHelper::isPressPadLeft()) {
        isEnabled = !isEnabled;
    }
    // only use draw queue if enabled
    if(isEnabled) {
        for (size_t i = 0; i < drawQueueCount; i++) {
            drawQueue[i]();
        }
    }

    ImGui::Render();
    ImguiNvnBackend::renderDrawData(ImGui::GetDrawData());
}

void nvnImGui::InstallHooks() {
    NvnBootstrapHook::InstallAtSymbol("nvnBootstrapLoader");
    DisableFullKeyState::InstallAtSymbol("_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_16NpadFullKeyStateEiRKj");
    DisableHandheldState::InstallAtSymbol("_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_17NpadHandheldStateEiRKj");
    DisableJoyDualState::InstallAtSymbol("_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_16NpadJoyDualStateEiRKj");
    DisableJoyLeftState::InstallAtSymbol("_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_16NpadJoyLeftStateEiRKj");
    DisableJoyRightState::InstallAtSymbol("_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_17NpadJoyRightStateEiRKj");
}

bool nvnImGui::InitImGui() {
    if (nvnDevice && nvnQueue && nvnCmdBuf) {

        Logger::log("Creating ImGui.\n");

        IMGUI_CHECKVERSION();

        ImGuiMemAllocFunc allocFunc = [](size_t size, void *user_data) {
            return nn::init::GetAllocator()->Allocate(size);
        };

        ImGuiMemFreeFunc freeFunc = [](void *ptr, void *user_data) {
            nn::init::GetAllocator()->Free(ptr);
        };

        ImGui::SetAllocatorFunctions(allocFunc, freeFunc, nullptr);

        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;

        ImGui::StyleColorsDark();

        ImguiNvnBackend::NvnBackendInitInfo initInfo = {
                .device = nvnDevice,
                .queue = nvnQueue,
                .cmdBuf = nvnCmdBuf
        };

        ImguiNvnBackend::InitBackend(initInfo);

        InputHelper::initKBM();

        InputHelper::setPort(0); // set input helpers default port to zero


#if IMGUI_USEEXAMPLE_DRAW
        IMGUINVN_DRAWFUNC(
                ImGui::ShowDemoWindow();
            //    ImGui::ShowStyleSelector("Style Selector");
            //        ImGui::ShowMetricsWindow();
            //        ImGui::ShowDebugLogWindow();
            //        ImGui::ShowStackToolWindow();
            //        ImGui::ShowAboutWindow();
            //        ImGui::ShowFontSelector("Font Selector");
            //        ImGui::ShowUserGuide();
        )
#endif

        return true;

    } else {
        Logger::log("Unable to create ImGui Renderer!\n");

        return false;
    }
}