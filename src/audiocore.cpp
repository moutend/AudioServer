#include <cpplogger/cpplogger.h>
#include <cstdint>
#include <sstream>
#include <strsafe.h>

#include <windows.h>

// These headers must be included after windows.h.
#include <avrt.h>
#include <functiondiscoverykeys.h>

#include "audiocore.h"
#include "util.h"

using namespace Windows::Media::Devices;

extern Logger::Logger *Log;

AudioCore::AudioCore(PCMAudio::Engine *engine, HANDLE refreshEvent,
                     HANDLE failEvent, HANDLE nextEvent)
    : mEngine(engine), mRefreshEvent(refreshEvent), mFailEvent(failEvent),
      mNextEvent(nextEvent) {}

void AudioCore::LogMixFormat() {
  std::wstringstream wss;

  wss << L"MixFormat = {";
  wss << L"nChannels: " << mMixFormat->nChannels << L", ";
  wss << L"nSamplesPerSec: " << mMixFormat->nSamplesPerSec << L", ";
  wss << L"wBitsPerSample: " << mMixFormat->wBitsPerSample << L", ";

  WAVEFORMATEXTENSIBLE *mixFormatEx =
      reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mMixFormat);

  wss << L"wValidBitsPerSample: " << mixFormatEx->Samples.wValidBitsPerSample
      << L"}";

  Log->Info(wss.str(), GetCurrentThreadId(), __LONGFILE__);
}

void AudioCore::Shutdown() {
  if (!mActive) {
    return;
  }

  Log->Info(L"Shutdown audio core", GetCurrentThreadId(), __LONGFILE__);

  if (!SetEvent(mShutdownEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return;
  }

  WaitForSingleObject(mRenderThread, INFINITE);
  SafeCloseHandle(&mRenderThread);

  Log->Info(L"Delete audio render thread", GetCurrentThreadId(), __LONGFILE__);

  CoTaskMemFree(mMixFormat);
  mMixFormat = nullptr;

  SafeCloseHandle(&mShutdownEvent);
  SafeCloseHandle(&mRenderEvent);
  SafeCloseHandle(&mSwitchStreamEvent);
  SafeRelease(&mDevice);
  SafeRelease(&mDeviceEnumerator);
  SafeRelease(&mAudioRenderClient);
  SafeRelease(&mAudioClient);

  mActive = false;
}

HRESULT
AudioCore::ActivateCompleted(IActivateAudioInterfaceAsyncOperation *operation) {
  HRESULT hr{S_OK};
  HRESULT hrActivateResult{S_OK};
  IUnknown *unknown{nullptr};

  hr = operation->GetActivateResult(&hrActivateResult, &unknown);

  if (!SUCCEEDED(hr) || !SUCCEEDED(hrActivateResult)) {
    Log->Fail(L"Failed to call "
              "IActivateAudioInterfaceAsyncOperation::GetActivateResult",
              GetCurrentThreadId(), __LONGFILE__);
    hr = E_FAIL;
    goto CLEANUP;
  }

  unknown->QueryInterface(IID_PPV_ARGS(&mAudioClient));

  if (mAudioClient == nullptr) {
    Log->Fail(L"Failed to get mAudioClient", GetCurrentThreadId(),
              __LONGFILE__);
    hr = E_FAIL;
    goto CLEANUP;
  }

  hr = mAudioClient->GetMixFormat(&mMixFormat);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IAudioClient::GetMixFormat",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }
  if (reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mMixFormat)->SubFormat !=
      KSDATAFORMAT_SUBTYPE_PCM) {
    reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mMixFormat)->SubFormat =
        KSDATAFORMAT_SUBTYPE_PCM;
  }

  mFrameSize = mMixFormat->nBlockAlign;

  LogMixFormat();

  hr = mAudioClient->Initialize(
      AUDCLNT_SHAREMODE_SHARED,
      AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
      mHNSBufferDuration, mHNSBufferDuration, mMixFormat, nullptr);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to initialize mAudioClient", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  hr = mAudioClient->GetBufferSize(&mBufferFrames);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IAudioClient::GetBufferSize",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = mAudioClient->GetService(__uuidof(IAudioRenderClient),
                                (void **)&mAudioRenderClient);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to get mAudioRenderClient", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  mRenderEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mRenderEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    hr = E_FAIL;
    goto CLEANUP;
  }

  mShutdownEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mShutdownEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    hr = E_FAIL;
    goto CLEANUP;
  }

  mSwitchStreamEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mSwitchStreamEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    hr = E_FAIL;
    goto CLEANUP;
  }

  hr = mAudioClient->SetEventHandle(mRenderEvent);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IAudioClient::SetEventHandle",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  Log->Info(L"Create audio render thread", GetCurrentThreadId(), __LONGFILE__);

  mRenderThread = CreateThread(nullptr, 0, RenderThread, this, 0, nullptr);

  if (mRenderThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                        IID_PPV_ARGS(&mDeviceEnumerator));

  if (FAILED(hr)) {
    Log->Fail(L"Failed to create mDeviceEnumerator", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  hr = mDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &mDevice);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to create mDevice", GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  IPropertyStore *ps;

  hr = mDevice->OpenPropertyStore(STGM_READ, &ps);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to open property store", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  PROPVARIANT friendlyName;
  PropVariantInit(&friendlyName);

  hr = ps->GetValue(PKEY_Device_FriendlyName, &friendlyName);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to get friendly name", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  wchar_t *deviceId = new wchar_t[256]{};

  hr = mDevice->GetId(&deviceId);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IMMDevice::GetId", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  constexpr size_t deviceNameLen{512};
  wchar_t *deviceName = new wchar_t[deviceNameLen]{};

  hr = StringCbPrintfW(
      deviceName, deviceNameLen, L"Default device name: %s (DeviceId=%s)",
      friendlyName.vt != VT_LPWSTR ? L"Unknown" : friendlyName.pwszVal,
      deviceId);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to read friendly name", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  Log->Info(deviceName, GetCurrentThreadId(), __LONGFILE__);

  PropVariantClear(&friendlyName);

  mNotification = new Notification(mSwitchStreamEvent, mDeviceRole, deviceId);
  hr = mDeviceEnumerator->RegisterEndpointNotificationCallback(mNotification);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call "
              L"DeviceEnumerator::RegisterEndpointNotificationCallback",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = mAudioClient->Start();

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IAudioClient::Start", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  mActive = true;
  Log->Info(L"Complete initialize audio core", GetCurrentThreadId(),
            __LONGFILE__);

CLEANUP:

  if (FAILED(hr)) {
    SafeCloseHandle(&mRenderThread);
    SafeCloseHandle(&mRenderEvent);
    SafeCloseHandle(&mSwitchStreamEvent);
    SafeCloseHandle(&mShutdownEvent);
    SafeRelease(&mDeviceEnumerator);
    SafeRelease(&mAudioRenderClient);
    SafeRelease(&mAudioClient);
    SafeRelease(&unknown);

    if (!SetEvent(mFailEvent)) {
      Log->Fail(L"Failed to ", GetCurrentThreadId(), __LONGFILE__);
    }
  }

  // Always must return S_OK.
  return S_OK;
}

DWORD AudioCore::RenderThread(LPVOID Context) {
  AudioCore *renderer = static_cast<AudioCore *>(Context);

  return renderer->DoRenderThread();
}

DWORD AudioCore::DoRenderThread() {
  Log->Info(L"Start audio render thread", GetCurrentThreadId(), __LONGFILE__);

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call CoInitializeEx", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }

  HANDLE mmcssHandle{nullptr};
  HANDLE waitArray[3] = {mShutdownEvent, mSwitchStreamEvent, mRenderEvent};
  DWORD mmcssTaskIndex{0};

  if (!mDisableMMCSS) {
    mmcssHandle = AvSetMmThreadCharacteristics("Audio", &mmcssTaskIndex);
    if (mmcssHandle == nullptr) {
      Log->Warn(L"Failed to call AvSetMmThreadCharacteristics",
                GetCurrentThreadId(), __LONGFILE__);
    } else {
      Log->Info(L"Success applying MMCSS attribute", GetCurrentThreadId(),
                __LONGFILE__);
    }
  }

  int bytesPerSample{mMixFormat->wBitsPerSample / 8};
  bool isPlaying{true};

  mEngine->SetTargetSamplesPerSec(mMixFormat->nSamplesPerSec);

  while (isPlaying) {
    DWORD waitResult = WaitForMultipleObjects(3, waitArray, FALSE, INFINITE);

    switch (waitResult) {
    case WAIT_OBJECT_0 + 0: // mShutdownEvent
      Log->Info(L"Received shutdown event", GetCurrentThreadId(), __LONGFILE__);
      isPlaying = false;
      break;
    case WAIT_OBJECT_0 + 1: // mSwitchStreamEvent
      hr = mDeviceEnumerator->UnregisterEndpointNotificationCallback(
          mNotification);

      if (FAILED(hr)) {
        Log->Fail(L"Failed to call "
                  "IMMDeviceEnumerator::UnregisterEndpointNotificationCallback",
                  GetCurrentThreadId(), __LONGFILE__);
        break;
      }

      Log->Info(L"Switch render device", GetCurrentThreadId(), __LONGFILE__);
      isPlaying = false;
      break;
    case WAIT_OBJECT_0 + 2: // mRenderEvent
      BYTE *pData{nullptr};
      UINT32 padding{};
      UINT32 availableFrames{};

      hr = mAudioClient->GetCurrentPadding(&padding);

      if (!SUCCEEDED(hr)) {
        Log->Warn(L"Failed to call IAudioClient::GetCurrentPadding",
                  GetCurrentThreadId(), __LONGFILE__);
        isPlaying = false;
        break;
      }

      availableFrames = mBufferFrames - padding;

      hr = mAudioRenderClient->GetBuffer(availableFrames, &pData);

      if (!SUCCEEDED(hr)) {
        Log->Warn(L"Failed to call IAudioRenderClient::GetBuffer",
                  GetCurrentThreadId(), __LONGFILE__);
        isPlaying = false;
        break;
      }

      int samples =
          static_cast<int>(availableFrames * mFrameSize) / bytesPerSample;

      for (int i = 0; i < samples; i++) {
        double f64 = mEngine->Read();
        int32_t s32 = static_cast<int32_t>(f64);

        for (int j = 0; j < bytesPerSample; j++) {
          pData[bytesPerSample * i + bytesPerSample - 1 - j] =
              s32 >> (8 * (bytesPerSample - 1 - j)) & 0xFF;
        }

        mEngine->Next();

        if (!mEngine->IsCompleted()) {
          continue;
        }
        if (!SetEvent(mNextEvent)) {
          Log->Fail(L"Failed to send event", GetCurrentThreadId(),
                    __LONGFILE__);
        }

        mEngine->Reset();
      }

      hr = mAudioRenderClient->ReleaseBuffer(availableFrames, 0);

      if (!SUCCEEDED(hr)) {
        Log->Warn(L"Failed to call IAudioRenderClient::releaseBuffer",
                  GetCurrentThreadId(), __LONGFILE__);
        isPlaying = false;
        break;
      }

      break;
    }
  }
  if (!mDisableMMCSS) {
    AvRevertMmThreadCharacteristics(mmcssHandle);
  }

  hr = mAudioClient->Stop();

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IAudioClient::Stop", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }
  if (!SetEvent(mRefreshEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  CoUninitialize();

  Log->Info(L"End audio render thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
