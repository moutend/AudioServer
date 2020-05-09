#pragma once

#include <IAudioServer/IAudioServer.h>
#include <cppaudio/engine.h>
#include <windows.h>

#include "types.h"

struct LoggingContext {
  HANDLE QuitEvent = nullptr;
};

struct VoiceProperty {
  wchar_t *Id = nullptr;
  wchar_t *DisplayName = nullptr;
  wchar_t *Language = nullptr;
  size_t DisplayNameLength;
  size_t LanguageLength;
  double SpeakingRate = 1.0;
  double AudioVolume = 1.0;
  double AudioPitch = 1.0;
};

struct VoiceInfoContext {
  unsigned int Count = 0;
  unsigned int DefaultVoiceIndex = 0;
  VoiceProperty **VoiceProperties = nullptr;
};

struct VoiceContext {
  HANDLE QuitEvent = nullptr;
  HANDLE KickEvent = nullptr;
  bool IsSSML = false;
  wchar_t *Text = nullptr;
  VoiceInfoContext *VoiceInfoCtx = nullptr;
  PCMAudio::KickEngine *Engine = nullptr;
};

struct SFXContext {
  HANDLE QuitEvent = nullptr;
  HANDLE KickEvent = nullptr;
  int16_t SFXIndex = 0;
  double Pan = 0.0;
  double SleepDuration = 0.0; /* ms */
  PCMAudio::KickEngine *Engine = nullptr;
};

struct CommandContext {
  HANDLE QuitEvent = nullptr;
  HANDLE PushEvent = nullptr;
  HANDLE NextEvent = nullptr;
  VoiceContext *VoiceCtx = nullptr;
  SFXContext *SFXCtx = nullptr;
  Command **Commands = nullptr;
  bool IsIdle = true;
  int32_t ReadIndex = 0;
  int32_t WriteIndex = 0;
  int32_t MaxCommands = 256;
  NotifyIdleStateHandler NotifyIdleState = nullptr;
  PCMAudio::Engine *Engine = nullptr;
};

struct RenderContext {
  HANDLE QuitEvent = nullptr;
  HANDLE NextEvent = nullptr;
  PCMAudio::Engine *Engine = nullptr;
};
