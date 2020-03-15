#pragma once

#include <AudioClient.h>
#include <AudioPolicy.h>
#include <MMDeviceAPI.h>
#include <cppaudio/engine.h>
#include <windows.h>
#include <wrl/implements.h>

#include "notification.h"

using namespace Microsoft::WRL;

class AudioCore
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, FtmBase,
                          IActivateAudioInterfaceCompletionHandler> {
public:
  AudioCore(PCMAudio::Engine *engine, HANDLE refreshEvent, HANDLE failEvent,
            HANDLE nextEvent);

  void LogMixFormat();
  void Shutdown();

  STDMETHOD(ActivateCompleted)
  (IActivateAudioInterfaceAsyncOperation *operation);

  static DWORD __stdcall RenderThread(LPVOID Context);
  DWORD DoRenderThread();

private:
  bool mActive = false;
  PCMAudio::Engine *mEngine = nullptr;

  ERole mDeviceRole;
  bool mDisableMMCSS;
  bool mInStreamSwitch;

  HANDLE mRenderThread = nullptr;

  HANDLE mRefreshEvent = nullptr;
  HANDLE mFailEvent = nullptr;
  HANDLE mNextEvent = nullptr;

  HANDLE mRenderEvent = nullptr;
  HANDLE mShutdownEvent = nullptr;
  HANDLE mSwitchStreamEvent = nullptr;

  Notification *mNotification = nullptr;

  IMMDeviceEnumerator *mDeviceEnumerator = nullptr;
  IMMDevice *mDevice = nullptr;
  IAudioRenderClient *mAudioRenderClient = nullptr;
  IAudioClient3 *mAudioClient = nullptr;
  WAVEFORMATEX *mMixFormat = nullptr;

  REFERENCE_TIME mHNSBufferDuration = 0;
  UINT32 mDefaultPeriodInFrames;
  UINT32 mFundamentalPeriodInFrames;
  UINT32 mMaxPeriodInFrames;
  UINT32 mMinPeriodInFrames;
  UINT32 mBufferFrames;
  UINT32 mFrameSize;
};
