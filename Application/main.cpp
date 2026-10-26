#include "GameApplication.h"
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR commandLine, _In_ int) {
    GameApplication game;
    return game.Run(commandLine);
}