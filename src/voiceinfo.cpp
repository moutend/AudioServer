#include <cpplogger/cpplogger.h>
#include <cstring>
#include <ppltasks.h>
#include <roapi.h>
#include <robuffer.h>
#include <wrl.h>

#include "context.h"
#include "util.h"
#include "voiceinfo.h"

using namespace Microsoft::WRL;
using namespace Windows::Media::SpeechSynthesis;
using namespace concurrency;
using namespace Windows::Storage::Streams;
using namespace Windows::Media;

using Windows::Foundation::Metadata::ApiInformation;

extern Logger::Logger *Log;

DWORD WINAPI voiceInfo(LPVOID context) {
  Log->Info(L"Start Voice info thread", GetCurrentThreadId(), __LONGFILE__);

  VoiceInfoContext *ctx = static_cast<VoiceInfoContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain ctx", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  RoInitialize(RO_INIT_MULTITHREADED);

  auto synth = ref new SpeechSynthesizer();

  ctx->Count = synth->AllVoices->Size;
  ctx->VoiceProperties = new VoiceProperty *[synth->AllVoices->Size];

  unsigned int defaultVoiceIndex{};
  VoiceInformation ^ defaultInfo = synth->DefaultVoice;

  for (unsigned int i = 0; i < ctx->Count; ++i) {
    synth->Voice = synth->AllVoices->GetAt(i);

    ctx->VoiceProperties[i] = new VoiceProperty();

    size_t idLength = wcslen(synth->Voice->Id->Data());
    ctx->VoiceProperties[i]->Id = new wchar_t[idLength + 1]{};
    std::wmemcpy(ctx->VoiceProperties[i]->Id, synth->Voice->Id->Data(),
                 idLength);

    size_t displayNameLength = wcslen(synth->Voice->DisplayName->Data());
    ctx->VoiceProperties[i]->DisplayName = new wchar_t[displayNameLength + 1]{};
    std::wmemcpy(ctx->VoiceProperties[i]->DisplayName,
                 synth->Voice->DisplayName->Data(), displayNameLength);

    size_t languageLength = wcslen(synth->Voice->Language->Data());
    ctx->VoiceProperties[i]->Language = new wchar_t[languageLength + 1]{};
    std::wmemcpy(ctx->VoiceProperties[i]->Language,
                 synth->Voice->Language->Data(), languageLength);

    ctx->VoiceProperties[i]->SpeakingRate = synth->Options->SpeakingRate;
    ctx->VoiceProperties[i]->AudioPitch = synth->Options->AudioPitch;
    ctx->VoiceProperties[i]->AudioVolume = synth->Options->AudioVolume;

    if (defaultInfo->Id->Equals(synth->Voice->Id)) {
      defaultVoiceIndex = i;
    }
  }

  ctx->DefaultVoiceIndex = defaultVoiceIndex;

  RoUninitialize();

  Log->Info(L"End Voice info thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
