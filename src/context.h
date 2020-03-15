#pragma once

#include <cppaudio/engine.h>
#include <windows.h>

#include "types.h"

struct LogLoopContext {
  HANDLE QuitEvent = nullptr;
};

struct VoiceProperty {
  wchar_t *Id = nullptr;
  wchar_t *DisplayName = nullptr;
  wchar_t *Language = nullptr;
  double SpeakingRate = 1.0;
  double AudioVolume = 1.0;
  double AudioPitch = 1.0;
};

struct VoiceInfoContext {
  unsigned int Count = 0;
  unsigned int DefaultVoiceIndex = 0;
  VoiceProperty **VoiceProperties = nullptr;
};

struct VoiceLoopContext {
  HANDLE FeedEvent = nullptr;
  HANDLE NextEvent = nullptr;
  HANDLE QuitEvent = nullptr;
  bool IsSSML = false;
  wchar_t *Text = nullptr;
  PCMAudio::RingEngine *VoiceEngine = nullptr;
  VoiceInfoContext *VoiceInfoCtx = nullptr;
};

struct SFXLoopContext {
  HANDLE FeedEvent = nullptr;
  HANDLE NextEvent = nullptr;
  HANDLE QuitEvent = nullptr;
  int16_t SFXIndex = 0;
  double WaitDuration = 0.0;
  PCMAudio::LauncherEngine *SFXEngine = nullptr;
};

struct CommandLoopContext {
  HANDLE PushEvent = nullptr;
  HANDLE QuitEvent = nullptr;
  VoiceLoopContext *VoiceLoopCtx = nullptr;
  SFXLoopContext *SFXLoopCtx = nullptr;
  Command **Commands = nullptr;
  bool IsIdle = true;
  int32_t ReadIndex = 0;
  int32_t WriteIndex = 0;
  int32_t MaxCommands = 256;
};

struct AudioLoopContext {
  HANDLE NextEvent = nullptr;
  HANDLE QuitEvent = nullptr;
  PCMAudio::Engine *Engine = nullptr;
};
