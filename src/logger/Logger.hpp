#pragma once

#include "nn/result.h"
#include <nn/socket.hpp>

#include "ui/ImGuiDebugConsole.h"

enum class LoggerType {
    None,
    Network,
    Emulator,
    ImGui
};

class Logger {

    enum class LogSeverity {
        Info,
        Warn,
        Error
    };

    enum class NetworkState {
        UNINITIALIZED = 0,
        CONNECTED = 1,
        UNAVAILABLE = 2,
        DISCONNECTED = 3
    };

    typedef void (*SendLogCallback)(const char *msg, size_t size, LogSeverity severity);

    // general info
    NetworkState mState = NetworkState::UNINITIALIZED;
    LoggerType mType = LoggerType::None;
    SendLogCallback mLogCallback = nullptr;

    // network
    int mSocketFd = -1;
    in_addr hostAddress = {};
    sockaddr serverAddress = {};
    // imgui
    ImGuiUI::DebugConsole mDbgConsole = ImGuiUI::DebugConsole();

    /// Outputs log to the currently set log type (Network, Emulator, ImGui)
    static void sendLog(const char *msg, size_t msgLen, LogSeverity severity = LogSeverity::Info);

    // network funcs
    nn::Result initNetwork(const char *ip, u16 port);

public:

    Logger() = default;

    static NOINLINE Logger &instance();

    static void setLogType(LoggerType type);

    static void log(const char *fmt, ...);

    static void log(const char *fmt, va_list args, LogSeverity severity = LogSeverity::Info);

    static void logWarn(const char *fmt, ...);

    static void logError(const char *fmt, ...);

    static void logLine(const char *fmt, ...);

};