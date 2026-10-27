#include "Engine/Audio/Audio.h"
#include <iostream>
#include <objbase.h>
#include <stdexcept>
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
int main() {
  const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  auto *audio = Audio::GetInstance();
  try {
    audio->Initialize();
    auto sound = audio->SoundLoadWave("Resources/Audio/Selected/landing.wav");
    auto first = audio->PlayVoice(sound, true);
    audio->SetVoiceVolume(first, 0);
    Check(first != 0 && audio->GetVoiceSnapshot(first).active,
          "Looping voice must be tracked");
    audio->PauseVoice(first, true);
    Check(audio->GetVoiceSnapshot(first).paused,
          "Pause retains the queued voice");
    audio->PauseVoice(first, false);
    Check(!audio->GetVoiceSnapshot(first).paused &&
              audio->GetVoiceSnapshot(first).active,
          "Resume must continue the same voice");
    audio->StopVoice(first);
    Check(!audio->GetVoiceSnapshot(first).active,
          "Stopped voice must become invalid");
    auto second = audio->PlayVoice(sound, true);
    audio->SetVoiceVolume(second, 0);
    Check(second != first,
          "A recycled XAudio pointer must receive a fresh stable ID");
    audio->StopVoice(first);
    audio->PauseVoice(first, true);
    audio->SetVoiceVolume(first, 0);
    audio->SetVoicePitch(first, 2);
    Check(audio->GetVoiceSnapshot(second).active &&
              !audio->GetVoiceSnapshot(second).paused,
          "Stale IDs must not affect another layer");
    audio->SoundUnload(&sound);
    Check(audio->GetVoiceSnapshot(second).active,
          "Active voice must own its sample buffer");
    audio->StopVoice(second);
    audio->Finalize();
    if (SUCCEEDED(com))
      CoUninitialize();
    std::cout << "PASS: stable voice IDs, stale-handle isolation, loop "
                 "ownership, pause and resume.\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    audio->Finalize();
    if (SUCCEEDED(com))
      CoUninitialize();
    return 1;
  }
}
