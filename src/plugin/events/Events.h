#pragma once

#include "plugin/events/EventHolders.h"
#include <game/HakoniwaSequence/HakoniwaSequence.h>
#include <game/System/GameSystem.h>
#include <game/Player/PlayerActorHakoniwa.h>

namespace GameSystemEvent {
    CREATE_HOOK_EVENT_ARGS(Init, "_ZN10GameSystem4initEv", GameSystem*)

    void ALWAYS_INLINE installEvents() {
        INSTALL_EVENT(Init, Symbol);
    }
    void ALWAYS_INLINE removeFromEvents(const char* key) {

    }
    void ALWAYS_INLINE clearEvents() {
        Init::clearEvents();
    }

}

namespace HakoniwaSequenceEvent {
    CREATE_HOOK_EVENT_ARGS(Init, "_ZN16HakoniwaSequence4initERKN2al16SequenceInitInfoE", HakoniwaSequence*, al::SequenceInitInfo const&)
    CREATE_HOOK_EVENT_ARGS(Update, "_ZN16HakoniwaSequence6updateEv", HakoniwaSequence*)
    // Nerves
    CREATE_HOOK_EVENT_ARGS(Destroy, "_ZN16HakoniwaSequence10exeDestroyEv", HakoniwaSequence*)

    void ALWAYS_INLINE installEvents() {
        INSTALL_EVENT(Init, Symbol);
        INSTALL_EVENT(Update, Symbol);
        INSTALL_EVENT(Destroy, Symbol);
    }

    void ALWAYS_INLINE removeFromEvents(const char* key) {

    }
    void ALWAYS_INLINE clearEvents() {
        Update::clearEvents();
        Destroy::clearEvents();
        Init::clearEvents();
    }
}

namespace StageSceneEvent {
    CREATE_HOOK_EVENT_ARGS(Init, "_ZN10StageScene4initERKN2al13SceneInitInfoE", StageScene*, al::SceneInitInfo const&)
    CREATE_HOOK_EVENT_ARGS(Control, "_ZN10StageScene7controlEv", StageScene*)
    CREATE_HOOK_EVENT_ARGS(Kill, "_ZN10StageScene4killEv", StageScene*)
    CREATE_HOOK_EVENT_ARGSR(UpdatePlay, "_ZN10StageScene10updatePlayEv", bool, StageScene*)
    // Nerves

    void ALWAYS_INLINE installEvents() {
        INSTALL_EVENT(Init, Symbol);
        INSTALL_EVENT(Control, Symbol);
        INSTALL_EVENT(Kill, Symbol);
        INSTALL_EVENT(UpdatePlay, Symbol);
    }

    void ALWAYS_INLINE removeFromEvents(const char* key) {

    }
    void ALWAYS_INLINE clearEvents() {
        Init::clearEvents();
        Control::clearEvents();
        Kill::clearEvents();
        UpdatePlay::clearEvents();
    }
}

namespace PlayerEvent {
    CREATE_HOOK_EVENT_ARGS(Movement, "_ZN19PlayerActorHakoniwa8movementEv", PlayerActorHakoniwa*)
    CREATE_HOOK_EVENT_ARGSR(ReceiveMsg, "_ZN19PlayerActorHakoniwa10receiveMsgEPKN2al9SensorMsgEPNS0_9HitSensorES5_", bool,
                            PlayerActorHakoniwa*, al::SensorMsg*, al::HitSensor*, al::HitSensor*)
    CREATE_HOOK_EVENT_ARGS(Init, "_ZN19PlayerActorHakoniwa10initPlayerERKN2al13ActorInitInfoERK14PlayerInitInfo", PlayerActorHakoniwa*, al::ActorInitInfo const&, PlayerInitInfo const&)

    void ALWAYS_INLINE installEvents() {
        INSTALL_EVENT(Movement, Symbol);
        INSTALL_EVENT(ReceiveMsg, Symbol);
        INSTALL_EVENT(Init, Symbol);
    }

    void ALWAYS_INLINE removeFromEvents(const char* key) {

    }

    void ALWAYS_INLINE clearEvents() {
        Movement::clearEvents();
        ReceiveMsg::clearEvents();
        Init::clearEvents();
    }

}

namespace ModEvent {
    CREATE_MOD_EVENT(ImguiDraw)
    CREATE_MOD_EVENT(OnPlayerDamage)

    void ALWAYS_INLINE removeFromEvents(const char* key) {
        ImguiDraw::RemoveEvents(key);
        OnPlayerDamage::RemoveEvents(key);
    }
    void ALWAYS_INLINE clearEvents() {
        ImguiDraw::RemoveAllEvents();
        OnPlayerDamage::RemoveAllEvents();
    }
}

namespace EventSystem {
    void ALWAYS_INLINE installAllEvents() {
        HakoniwaSequenceEvent::installEvents();
        GameSystemEvent::installEvents();
        StageSceneEvent::installEvents();
        PlayerEvent::installEvents();
    }

    void ALWAYS_INLINE clearAllEvents() {
        Logger::log("Clearing Events\n");

        ModEvent::clearEvents();
        HakoniwaSequenceEvent::clearEvents();
        GameSystemEvent::clearEvents();
        StageSceneEvent::clearEvents();
        PlayerEvent::clearEvents();
    }

    void ALWAYS_INLINE removeFromEvents(const char* key) {
        Logger::log("Clearing Events for: %s\n", key);

        ModEvent::removeFromEvents(key);
        HakoniwaSequenceEvent::removeFromEvents(key);
        GameSystemEvent::removeFromEvents(key);
        StageSceneEvent::removeFromEvents(key);
        PlayerEvent::removeFromEvents(key);
    }
}