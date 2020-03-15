#include <cppaudio/engine.h>
#include <cpplogger/cpplogger.h>
#include <cstring>
#include <ppltasks.h>
#include <roapi.h>
#include <robuffer.h>
#include <wrl.h>

#include "context.h"
#include "util.h"
#include "voiceloop.h"

using namespace Microsoft::WRL;
using namespace Windows::Media::SpeechSynthesis;
using namespace concurrency;
using namespace Windows::Storage::Streams;
using namespace Windows::Media;

using Windows::Foundation::Metadata::ApiInformation;

extern Logger::Logger *Log;

DWORD WINAPI voiceLoop(LPVOID context) {
  Log->Info(L"Start Voice loop thread", GetCurrentThreadId(), __LONGFILE__);

  VoiceLoopContext *ctx = static_cast<VoiceLoopContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain ctx", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  RoInitialize(RO_INIT_MULTITHREADED);

  bool isActive{true};
  auto synth = ref new SpeechSynthesizer();

  while (isActive) {
    HANDLE waitArray[2] = {ctx->FeedEvent, ctx->QuitEvent};
    DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);

    if (ctx->VoiceInfoCtx != nullptr &&
        ctx->VoiceInfoCtx->VoiceProperties != nullptr) {
      unsigned int index = ctx->VoiceInfoCtx->DefaultVoiceIndex;

      synth->Voice = synth->AllVoices->GetAt(index);
      synth->Options->SpeakingRate =
          ctx->VoiceInfoCtx->VoiceProperties[index]->SpeakingRate;
      synth->Options->AudioPitch =
          ctx->VoiceInfoCtx->VoiceProperties[index]->AudioPitch;
      synth->Options->AudioVolume =
          ctx->VoiceInfoCtx->VoiceProperties[index]->AudioVolume;
    }
    if (ApiInformation::IsApiContractPresent(
            "Windows.Foundation.UniversalApiContract", 6, 0)) {
      synth->Options->AppendedSilence = SpeechAppendedSilence::Min;
    }
    switch (waitResult) {
    case WAIT_OBJECT_0 + 0: // ctx->FeedEvent
      break;
    case WAIT_OBJECT_0 + 1: // ctx->QuitEvent
      isActive = false;
      continue;
    }

    task<SpeechSynthesisStream ^> speechTask;

    if (ctx->IsSSML) {
      try {
        Platform::String ^ ssml = ref new Platform::String(ctx->Text);
        speechTask = create_task(synth->SynthesizeSsmlToStreamAsync(ssml));
      } catch (Platform::Exception ^ e) {
        Log->Warn(L"Failed to call SynthesizeSsmlToStreamAsync",
                  GetCurrentThreadId(), __LONGFILE__);
        continue;
      }

      // Broken SSML string (e.g. unbalanced brackets) makes the task done.

      if (speechTask.is_done()) {
        Log->Warn(
            L"Failed to continue speech synthesis (probably SSML is broken)",
            GetCurrentThreadId(), __LONGFILE__);
        continue;
      }
    } else {
      try {
        Platform::String ^ text = ref new Platform::String(ctx->Text);
        speechTask = create_task(synth->SynthesizeTextToStreamAsync(text));
      } catch (Platform::Exception ^ e) {
        Log->Warn(L"Failed to call SynthesizeTextToStreamAsync",
                  GetCurrentThreadId(), __LONGFILE__);
        continue;
      }
    }

    int32_t waveLength{0};

    speechTask
        .then([&ctx, &waveLength](SpeechSynthesisStream ^ speechStream) {
          waveLength = static_cast<int32_t>(speechStream->Size);
          Buffer ^ buffer = ref new Buffer(waveLength);

          auto result = create_task(speechStream->ReadAsync(
              buffer, waveLength,
              Windows::Storage::Streams::InputStreamOptions::None));

          return result;
        })
        .then([&ctx, &waveLength](IBuffer ^ buffer) {
          if (waveLength > 0) {
            char *wave = getBytes(buffer);
            ctx->VoiceEngine->Feed(wave, waveLength);
          }
        })
        .then([](task<void> previous) {
          try {
            previous.get();
          } catch (Platform::Exception ^ e) {
            Log->Warn(L"Failed to complete speech synthesis",
                      GetCurrentThreadId(), __LONGFILE__);
          }
        })
        .wait();
  }

  RoUninitialize();

  Log->Info(L"End Voice loop thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
