#include "Logger.h"
#include <Windows.h>
#include <format>
#include <chrono>
#include <filesystem>

// 静的メンバ変数の実体
std::ofstream Logger::logStream_;

/*---------------------------------
初期化
-----------------------------------*/
void Logger::Initialize() {
    std::filesystem::create_directory("logs");
    auto now = std::chrono::system_clock::now();
    auto nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
    std::string logFilePath = "logs/" + dateString + ".log";

    logStream_.open(logFilePath);
}

/*------------------------------
終了処理
--------------------------------*/
void Logger::Finalize() {
    if (logStream_.is_open()) {
        logStream_.close();
    }
}

/*------------------------------
ログ出力
--------------------------------*/
void Logger::Log(const std::string& message) {
    if (logStream_.is_open()) {
        logStream_ << message << std::endl;
    }
    OutputDebugStringA(message.c_str());
}

// 文字変換
std::wstring Logger::ConvertString(const std::string& str) {
    if (str.empty()) return std::wstring();
    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0) return std::wstring();
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

std::string Logger::ConvertString(const std::wstring& str) {
    if (str.empty()) return std::string();
    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) return std::string();
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}