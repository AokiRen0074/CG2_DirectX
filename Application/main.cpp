#include "GameApplication.h"
#include <Windows.h>
#include <filesystem>
#include <iterator>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR commandLine, _In_ int) {
    wchar_t executable[32768]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, DWORD(std::size(executable)));
    if (length == 0 || length >= std::size(executable)) return 1;

    std::error_code error;
    std::filesystem::current_path(std::filesystem::path(executable).parent_path(), error);
    if (error) return 1;

    GameApplication application;
    return application.Run(commandLine);
}
