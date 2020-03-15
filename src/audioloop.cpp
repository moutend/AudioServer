#include <cpplogger/cpplogger.h>
#include <roapi.h>

#include "audiocore.h"
#include "audioloop.h"
#include "context.h"
#include "util.h"

using namespace Windows::Media::Devices;

extern Logger::Logger *Log;

DWORD WINAPI audioLoop(LPVOID context) {
  Log->Info(L"Start audio loop thread", GetCurrentThreadId(), __LONGFILE__);

  AudioLoopContext *ctx = static_cast<AudioLoopContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain ctx", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  RoInitialize(RO_INIT_MULTITHREADED);

  bool isActive{true};

  while (isActive) {
    HANDLE refreshEvent =
        CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

    if (refreshEvent == nullptr) {
      Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
      return E_FAIL;
    }

    HANDLE failEvent =
        CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

    if (failEvent == nullptr) {
      Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
      return E_FAIL;
    }

    Platform::String ^ deviceId = MediaDevice::GetDefaultAudioRenderId(
        Windows::Media::Devices::AudioDeviceRole::Default);

    IActivateAudioInterfaceAsyncOperation *op{nullptr};
    IActivateAudioInterfaceCompletionHandler *obj{nullptr};
    ComPtr<AudioCore> renderer =
        Make<AudioCore>(ctx->Engine, refreshEvent, failEvent, ctx->NextEvent);

    HRESULT hr = renderer->QueryInterface(IID_PPV_ARGS(&obj));

    if (FAILED(hr)) {
      Log->Fail(L"Failed to create instance of "
                "IActivateAudioInterfaceCompletionHandler",
                GetCurrentThreadId(), __LONGFILE__);
      return hr;
    }

    hr = ActivateAudioInterfaceAsync(deviceId->Data(), __uuidof(IAudioClient3),
                                     nullptr, obj, &op);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to call ActivateAudioInterfaceAsync",
                GetCurrentThreadId(), __LONGFILE__);
      return hr;
    }

    SafeRelease(&op);

    HANDLE waitArray[3] = {ctx->QuitEvent, refreshEvent, failEvent};
    DWORD waitResult = WaitForMultipleObjects(3, waitArray, FALSE, INFINITE);

    switch (waitResult) {
    case WAIT_OBJECT_0 + 0: // ctx->QuitEvent
      isActive = false;
      break;
    case WAIT_OBJECT_0 + 1: // refreshEvent
      Log->Info(L"Refresh audio renderer", GetCurrentThreadId(), __LONGFILE__);
      break;
    case WAIT_OBJECT_0 + 2: // failEvent
      Log->Warn(L"Failed to initialize audio renderer", GetCurrentThreadId(),
                __LONGFILE__);
      Sleep(1000);
      break;
    }

    SafeCloseHandle(&refreshEvent);
    SafeCloseHandle(&failEvent);

    renderer->Shutdown();
  }

  RoUninitialize();

  Log->Info(L"End audio loop thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
