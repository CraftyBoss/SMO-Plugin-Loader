#include "Logger.hpp"
#include "lib.hpp"
#include "nifm.h"
#include "socket.hpp"
#include "util.h"
#include <imgui_nvn.h>
#include <sys/fcntl.h>

Logger &Logger::instance() {
    static Logger instance;
    return instance;
}

nn::Result Logger::initNetwork(const char *ip, u16 port) {

    if (mState != NetworkState::UNINITIALIZED)
        return -1;

    if ((mSocketFd = nn::socket::Socket(2, 1, 6)) < 0) {
        mState = NetworkState::UNAVAILABLE;
        return -1;
    }

    nn::socket::Fcntl(mSocketFd, F_SETFL, O_NONBLOCK);

    nn::socket::InetAton(ip, &hostAddress);

    serverAddress.address = hostAddress;
    serverAddress.port = nn::socket::InetHtons(port);
    serverAddress.family = 2;

    nn::Result result = nn::socket::Connect(mSocketFd, &serverAddress, sizeof(serverAddress));

    if (!result.isSuccess() && nn::socket::GetLastErrno() != EINPROGRESS) {
        return -1;
    }

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(mSocketFd, &wfds);
    struct timeval tv = {};
    tv.tv_sec = 5;
    s32 selRes = nn::socket::Select(1, nullptr, &wfds, nullptr, &tv);
    if(selRes != 1)
        return -1;

    Logger::log("Connected!\n");

    mState = NetworkState::CONNECTED;

    return result;
}

void Logger::log(const char *fmt, va_list args, LogSeverity severity) {

    char buffer[0x500] = {};

    if (nn::util::VSNPrintf(buffer, sizeof(buffer), fmt, args) > 0) {
        sendLog(buffer, strlen(buffer), severity);
    }
}

void Logger::log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(fmt, args);
    va_end(args);
}

void Logger::logWarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if(instance().mType != LoggerType::ImGui) {

        constexpr const char *prefix = "\x1b[33m";
        constexpr size_t prefixLen = 5;
        constexpr const char *suffix = "\x1b[0m";
        constexpr size_t suffixLen = 5;

        char tempBuf[strlen(fmt)+prefixLen+suffixLen+1];
        sprintf(tempBuf, "%s%s", prefix, fmt);
        strcat(tempBuf, suffix);
        log(tempBuf, args, LogSeverity::Warn);
    }else {
        log(fmt, args, LogSeverity::Warn);
    }

    va_end(args);
}

void Logger::logError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if(instance().mType != LoggerType::ImGui) {

        constexpr const char *prefix = "\x1b[31m";
        constexpr size_t prefixLen = 5;
        constexpr const char *suffix = "\x1b[0m";
        constexpr size_t suffixLen = 5;

        char tempBuf[strlen(fmt)+prefixLen+suffixLen+1];
        sprintf(tempBuf, "%s%s", prefix, fmt);
        strcat(tempBuf, suffix);
        log(tempBuf, args, LogSeverity::Error);
    }else {
        log(fmt, args, LogSeverity::Error);
    }

    va_end(args);
}

void Logger::logLine(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);

    char buffer[0x500] = {};

    if (nn::util::VSNPrintf(buffer, sizeof(buffer), fmt, args) > 0) {

        size_t strLength = strlen(buffer);

        buffer[strLength++] = '\n'; // replace null terminator with newline
        buffer[strLength++] = '\0'; // add back null terminator

        sendLog(buffer, strlen(buffer));

    }

    va_end(args);
}

void Logger::setLogType(LoggerType type) {

    Logger &curInst = instance();

    curInst.mType = type;

    SendLogCallback logEmu = [](const char* msg, size_t msgLen, LogSeverity severity) { svcOutputDebugString(msg, msgLen); };
    SendLogCallback logNetwork = [](const char* msg, size_t msgLen, LogSeverity severity) {

        if (Logger::instance().mState != NetworkState::CONNECTED)
            return;

        nn::socket::Send(Logger::instance().mSocketFd, msg, msgLen, 0);

    };
    SendLogCallback logImGui = [](const char* msg, size_t, LogSeverity severity) {
        switch (severity) {
        case LogSeverity::Info:
            Logger::instance().mDbgConsole.LogInfo(msg);
            break;
        case LogSeverity::Warn:
            Logger::instance().mDbgConsole.LogWarn(msg);
            break;
        case LogSeverity::Error:
            Logger::instance().mDbgConsole.LogError(msg);
            break;
        }
    };

    switch (curInst.mType) {
    case LoggerType::Network:

        curInst.mLogCallback = logNetwork;

        if(curInst.mState == NetworkState::UNINITIALIZED)
            curInst.initNetwork(LOGGER_IP, 3080);

        break;
    case LoggerType::Emulator:
        curInst.mLogCallback = logEmu;
        break;
    case LoggerType::ImGui:

        curInst.mLogCallback = logImGui;

        static bool isAddedDraw = false;
        if(!isAddedDraw) {
            nvnImGui::addDrawFunc([]() {

                auto windowSize = ImVec2(700, 200);
                ImGuiIO& io = ImGui::GetIO();

                ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - windowSize.y), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

                instance().mDbgConsole.Draw("Logger Console");
            });
            isAddedDraw = true;
        }

        break;
    case LoggerType::None:
        break;
    }

    if(instance().mType != LoggerType::None)
        Logger::log("Logger Enabled.\n");
}

void Logger::sendLog(const char* msg, size_t msgLen, LogSeverity severity) {

    if(instance().mType != LoggerType::None)
        instance().mLogCallback(msg, msgLen, severity);
}