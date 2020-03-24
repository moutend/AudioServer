#include <cpplogger/cpplogger.h>
#include <cstring>
#include <fstream>
#include <mutex>
#include <shlwapi.h>
#include <strsafe.h>
#include <windows.h>

#include <strsafe.h>

#include "AudioServer.h"
#include <AudioServer/IAudioServer.h>

extern Logger::Logger *Log;

const TCHAR ProgIDStr[] = TEXT("AudioServer");
LONG LockCount{};
HINSTANCE AudioServerDLLInstance{};
TCHAR AudioServerCLSIDStr[256]{};
TCHAR LibraryIDStr[256]{};

// CAudioServer
CAudioServer::CAudioServer() : mReferenceCount(1), mTypeInfo(nullptr) {
  ITypeLib *pTypeLib{};
  HRESULT hr{};

  LockModule(true);

  hr = LoadRegTypeLib(LIBID_AudioServerLib, 1, 0, 0, &pTypeLib);

  if (SUCCEEDED(hr)) {
    pTypeLib->GetTypeInfoOfGuid(IID_IAudioServer, &mTypeInfo);
    pTypeLib->Release();
  }

  mVoiceInfoCtx = new VoiceInfoContext();

  mVoiceInfoThread = CreateThread(
      nullptr, 0, voiceInfo, static_cast<void *>(mVoiceInfoCtx), 0, nullptr);

  if (mVoiceInfoThread == nullptr) {
    return;
  }

  WaitForSingleObject(mVoiceInfoThread, INFINITE);
  SafeCloseHandle(&mVoiceInfoThread);
}

CAudioServer::~CAudioServer() {
  LockModule(false);

  for (unsigned int i = 0; i < mVoiceInfoCtx->Count; i++) {
    delete[] mVoiceInfoCtx->VoiceProperties[i]->Id;
    mVoiceInfoCtx->VoiceProperties[i]->Id = nullptr;

    delete[] mVoiceInfoCtx->VoiceProperties[i]->DisplayName;
    mVoiceInfoCtx->VoiceProperties[i]->DisplayName = nullptr;

    delete[] mVoiceInfoCtx->VoiceProperties[i]->Language;
    mVoiceInfoCtx->VoiceProperties[i]->Language = nullptr;
  }

  delete[] mVoiceInfoCtx->VoiceProperties;
  mVoiceInfoCtx->VoiceProperties = nullptr;

  delete mVoiceInfoCtx;
  mVoiceInfoCtx = nullptr;
}

STDMETHODIMP CAudioServer::QueryInterface(REFIID riid, void **ppvObject) {
  *ppvObject = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDispatch) ||
      IsEqualIID(riid, IID_IAudioServer)) {
    *ppvObject = static_cast<IAudioServer *>(this);
  } else {
    return E_NOINTERFACE;
  }

  AddRef();

  return S_OK;
}

STDMETHODIMP_(ULONG) CAudioServer::AddRef() {
  return InterlockedIncrement(&mReferenceCount);
}

STDMETHODIMP_(ULONG) CAudioServer::Release() {
  if (InterlockedDecrement(&mReferenceCount) == 0) {
    delete this;

    return 0;
  }

  return mReferenceCount;
}

STDMETHODIMP CAudioServer::GetTypeInfoCount(UINT *pctinfo) {
  *pctinfo = mTypeInfo != nullptr ? 1 : 0;

  return S_OK;
}

STDMETHODIMP CAudioServer::GetTypeInfo(UINT iTInfo, LCID lcid,
                                       ITypeInfo **ppTInfo) {
  if (mTypeInfo == nullptr) {
    return E_NOTIMPL;
  }

  mTypeInfo->AddRef();
  *ppTInfo = mTypeInfo;

  return S_OK;
}

STDMETHODIMP CAudioServer::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                         UINT cNames, LCID lcid,
                                         DISPID *rgDispId) {
  if (mTypeInfo == nullptr) {
    return E_NOTIMPL;
  }

  return mTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP CAudioServer::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                                  WORD wFlags, DISPPARAMS *pDispParams,
                                  VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                                  UINT *puArgErr) {
  void *p = static_cast<IAudioServer *>(this);

  if (mTypeInfo == nullptr) {
    return E_NOTIMPL;
  }

  return mTypeInfo->Invoke(p, dispIdMember, wFlags, pDispParams, pVarResult,
                           pExcepInfo, puArgErr);
}

STDMETHODIMP CAudioServer::Start() {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (mIsActive) {
    Log->Warn(L"Already initialized", GetCurrentThreadId(), __LONGFILE__);
    return S_OK;
  }

  mIsActive = true;

  Log = new Logger::Logger(L"AudioServer", L"v0.1.0-develop", 4096);

  Log->Info(L"Setup audio server", GetCurrentThreadId(), __LONGFILE__);

  mLogLoopCtx = new LogLoopContext();

  mLogLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mLogLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create log loop thread", GetCurrentThreadId(), __LONGFILE__);

  mLogLoopThread = CreateThread(nullptr, 0, logLoop,
                                static_cast<void *>(mLogLoopCtx), 0, nullptr);

  if (mLogLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mVoiceEngine = new PCMAudio::RingEngine();

  mNextVoiceEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mNextVoiceEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mVoiceLoopCtx = new VoiceLoopContext();
  mVoiceLoopCtx->NextEvent = mNextVoiceEvent;
  mVoiceLoopCtx->VoiceEngine = mVoiceEngine;
  mVoiceLoopCtx->VoiceInfoCtx = mVoiceInfoCtx;

  mVoiceLoopCtx->FeedEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mVoiceLoopCtx->FeedEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mVoiceLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mVoiceLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create voice loop thread", GetCurrentThreadId(), __LONGFILE__);

  mVoiceLoopThread = CreateThread(
      nullptr, 0, voiceLoop, static_cast<void *>(mVoiceLoopCtx), 0, nullptr);

  if (mVoiceLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mVoiceRenderCtx = new AudioLoopContext();
  mVoiceRenderCtx->NextEvent = mNextVoiceEvent;
  mVoiceRenderCtx->Engine = mVoiceEngine;

  mVoiceRenderCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mVoiceRenderCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create voice render thread", GetCurrentThreadId(), __LONGFILE__);

  mVoiceRenderThread = CreateThread(
      nullptr, 0, audioLoop, static_cast<void *>(mVoiceRenderCtx), 0, nullptr);

  if (mVoiceRenderThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mSFXEngine = new PCMAudio::LauncherEngine(mMaxWaves);

  for (int16_t i = 0; i < mMaxWaves; i++) {
    wchar_t *filePath = new wchar_t[256]{};

    HRESULT hr = StringCbPrintfW(filePath, 255, L"waves\\%03d.wav", i + 1);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to build file path", GetCurrentThreadId(),
                __LONGFILE__);
      continue;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::in);

    if (!mSFXEngine->Register(i, file)) {
      Log->Fail(L"Failed to register SFX", GetCurrentThreadId(), __LONGFILE__);
      continue;
    }

    Log->Info(filePath, GetCurrentThreadId(), __LONGFILE__);

    delete[] filePath;
    filePath = nullptr;

    file.close();
  }

  mNextSoundEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mNextSoundEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mSFXLoopCtx = new SFXLoopContext();
  mSFXLoopCtx->NextEvent = mNextSoundEvent;
  mSFXLoopCtx->SFXEngine = mSFXEngine;

  mSFXLoopCtx->FeedEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mSFXLoopCtx->FeedEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mSFXLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mSFXLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create sfx loop thread", GetCurrentThreadId(), __LONGFILE__);

  mSFXLoopThread = CreateThread(nullptr, 0, sfxLoop,
                                static_cast<void *>(mSFXLoopCtx), 0, nullptr);

  if (mSFXLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mSFXRenderCtx = new AudioLoopContext();
  mSFXRenderCtx->NextEvent = mNextSoundEvent;
  mSFXRenderCtx->Engine = mSFXEngine;

  mSFXRenderCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mSFXRenderCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create SFX render thread", GetCurrentThreadId(), __LONGFILE__);

  mSFXRenderThread = CreateThread(
      nullptr, 0, audioLoop, static_cast<void *>(mSFXRenderCtx), 0, nullptr);

  if (mSFXRenderThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mCommandLoopCtx = new CommandLoopContext();

  mCommandLoopCtx->PushEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mCommandLoopCtx->PushEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mCommandLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mCommandLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mCommandLoopCtx->VoiceLoopCtx = mVoiceLoopCtx;
  mCommandLoopCtx->SFXLoopCtx = mSFXLoopCtx;
  mCommandLoopCtx->Commands = new Command *[mCommandLoopCtx->MaxCommands] {};

  for (int32_t i = 0; i < mCommandLoopCtx->MaxCommands; i++) {
    mCommandLoopCtx->Commands[i] = new Command;
    mCommandLoopCtx->Commands[i]->Type = 0;
    mCommandLoopCtx->Commands[i]->SFXIndex = 0;
    mCommandLoopCtx->Commands[i]->WaitDuration = 0;
    mCommandLoopCtx->Commands[i]->Text = nullptr;
  }

  Log->Info(L"Create command loop thread", GetCurrentThreadId(), __LONGFILE__);

  mCommandLoopThread =
      CreateThread(nullptr, 0, commandLoop,
                   static_cast<void *>(mCommandLoopCtx), 0, nullptr);

  if (mCommandLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Complete setup AudioNode", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}

STDMETHODIMP CAudioServer::Stop() {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (!mIsActive) {
    return E_FAIL;
  }

  Log->Info(L"Stop audio server", GetCurrentThreadId(), __LONGFILE__);

  if (mCommandLoopThread == nullptr) {
    goto END_COMMANDLOOP_CLEANUP;
  }
  if (!SetEvent(mCommandLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mCommandLoopThread, INFINITE);
  SafeCloseHandle(&mCommandLoopThread);

  for (int32_t i = 0; i < mCommandLoopCtx->MaxCommands; i++) {
    delete[] mCommandLoopCtx->Commands[i]->Text;
    mCommandLoopCtx->Commands[i]->Text = nullptr;

    delete mCommandLoopCtx->Commands[i];
    mCommandLoopCtx->Commands[i] = nullptr;
  }

  SafeCloseHandle(&(mCommandLoopCtx->PushEvent));
  SafeCloseHandle(&(mCommandLoopCtx->QuitEvent));

  delete mCommandLoopCtx;
  mCommandLoopCtx = nullptr;

  Log->Info(L"Delete command loop thread", GetCurrentThreadId(), __LONGFILE__);

END_COMMANDLOOP_CLEANUP:

  if (mVoiceLoopThread == nullptr) {
    goto END_VOICELOOP_CLEANUP;
  }
  if (!SetEvent(mVoiceLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mVoiceLoopThread, INFINITE);
  SafeCloseHandle(&mVoiceLoopThread);

  SafeCloseHandle(&(mVoiceLoopCtx->QuitEvent));
  SafeCloseHandle(&(mVoiceLoopCtx->NextEvent));
  SafeCloseHandle(&(mVoiceLoopCtx->FeedEvent));

  delete mVoiceLoopCtx;
  mVoiceLoopCtx = nullptr;

  Log->Info(L"Delete voice loop thread", GetCurrentThreadId(), __LONGFILE__);

END_VOICELOOP_CLEANUP:

  if (mVoiceRenderThread == nullptr) {
    goto END_VOICERENDER_CLEANUP;
  }
  if (!SetEvent(mVoiceRenderCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mVoiceRenderThread, INFINITE);
  SafeCloseHandle(&mVoiceRenderThread);

  SafeCloseHandle(&(mVoiceRenderCtx->NextEvent));
  SafeCloseHandle(&(mVoiceRenderCtx->QuitEvent));

  delete mVoiceRenderCtx;
  mVoiceRenderCtx = nullptr;

  delete mVoiceEngine;
  mVoiceEngine = nullptr;

  Log->Info(L"Delete voice render thread", GetCurrentThreadId(), __LONGFILE__);

END_VOICERENDER_CLEANUP:

  if (mSFXLoopThread == nullptr) {
    goto END_SFXLOOP_CLEANUP;
  }
  if (!SetEvent(mSFXLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mSFXLoopThread, INFINITE);
  SafeCloseHandle(&mSFXLoopThread);

  SafeCloseHandle(&(mSFXLoopCtx->FeedEvent));
  SafeCloseHandle(&(mSFXLoopCtx->NextEvent));
  SafeCloseHandle(&(mSFXLoopCtx->QuitEvent));

  delete mSFXLoopCtx;
  mSFXLoopCtx = nullptr;

  Log->Info(L"Delete SFX loop thread", GetCurrentThreadId(), __LONGFILE__);

END_SFXLOOP_CLEANUP:

  if (mSFXRenderThread == nullptr) {
    goto END_SFXRENDER_CLEANUP;
  }
  if (!SetEvent(mSFXRenderCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mSFXRenderThread, INFINITE);
  SafeCloseHandle(&mSFXRenderThread);

  SafeCloseHandle(&(mSFXRenderCtx->NextEvent));
  SafeCloseHandle(&(mSFXRenderCtx->QuitEvent));

  delete mSFXEngine;
  mSFXEngine = nullptr;

  Log->Info(L"Delete SFX render thread", GetCurrentThreadId(), __LONGFILE__);

END_SFXRENDER_CLEANUP:

  SafeCloseHandle(&mNextVoiceEvent);
  SafeCloseHandle(&mNextSoundEvent);

  Log->Info(L"Complete Stop audio server", GetCurrentThreadId(), __LONGFILE__);

  if (mLogLoopThread == nullptr) {
    goto END_LOGLOOP_CLEANUP;
  }
  if (!SetEvent(mLogLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mLogLoopThread, INFINITE);
  SafeCloseHandle(&mLogLoopThread);

  SafeCloseHandle(&(mLogLoopCtx->QuitEvent));

  delete mLogLoopCtx;
  mLogLoopCtx = nullptr;

END_LOGLOOP_CLEANUP:

  mIsActive = false;

  return S_OK;
}

STDMETHODIMP CAudioServer::FadeIn() {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (!mIsActive) {
    return E_FAIL;
  }

  Log->Info(L"Called FadeIn()", GetCurrentThreadId(), __LONGFILE__);

  mVoiceEngine->FadeIn();
  mSFXEngine->FadeIn();

  return S_OK;
}

STDMETHODIMP CAudioServer::FadeOut() {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (!mIsActive) {
    return E_FAIL;
  }

  Log->Info(L"Called FadeOut()", GetCurrentThreadId(), __LONGFILE__);

  mVoiceEngine->FadeOut();
  mSFXEngine->FadeOut();

  return S_OK;
}

STDMETHODIMP CAudioServer::Push(RawCommand **pCommands, INT32 commandsLength,
                                INT32 isForcePush) {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (!mIsActive || pCommands == nullptr || commandsLength <= 0) {
    Log->Warn(L"Skipped IAudioServer::Push()", GetCurrentThreadId(),
              __LONGFILE__);
    return E_FAIL;
  }

  wchar_t *msg = new wchar_t[256]{};

  HRESULT hr = StringCbPrintfW(
      msg, 255, L"Called IAudioServer::Push() Read=%d,Write=%d,IsForce=%d",
      mCommandLoopCtx->ReadIndex, mCommandLoopCtx->WriteIndex, isForcePush);

  if (FAILED(hr)) {
    return E_FAIL;
  }

  Log->Info(msg, GetCurrentThreadId(), __LONGFILE__);

  delete[] msg;
  msg = nullptr;

  bool isIdle = mCommandLoopCtx->IsIdle;
  int32_t base = mCommandLoopCtx->WriteIndex;
  size_t textLen{};

  for (int32_t i = 0; i < commandsLength; i++) {
    int32_t offset = (base + i) % mCommandLoopCtx->MaxCommands;

    mCommandLoopCtx->Commands[offset]->Type = pCommands[i]->Type;

    switch (pCommands[i]->Type) {
    case 1:
      mCommandLoopCtx->Commands[offset]->SFXIndex =
          pCommands[i]->SFXIndex <= 0 ? 0 : pCommands[i]->SFXIndex - 1;
      break;
    case 2:
      mCommandLoopCtx->Commands[offset]->WaitDuration =
          pCommands[i]->WaitDuration;
      break;
    case 3: // Generate voice from plain text
    case 4: // Generate voice from SSML
      delete[] mCommandLoopCtx->Commands[offset]->Text;
      mCommandLoopCtx->Commands[offset]->Text = nullptr;

      textLen = std::wcslen(pCommands[i]->TextToSpeech);
      mCommandLoopCtx->Commands[offset]->Text = new wchar_t[textLen + 1]{};
      std::wmemcpy(mCommandLoopCtx->Commands[offset]->Text,
                   pCommands[i]->TextToSpeech, textLen);

      break;
    default:
      // do nothing
      continue;
    }

    mCommandLoopCtx->WriteIndex =
        (mCommandLoopCtx->WriteIndex + 1) % mCommandLoopCtx->MaxCommands;
  }
  if (isForcePush) {
    mCommandLoopCtx->ReadIndex = base;
  }
  if (!isForcePush && !isIdle) {
    return S_OK;
  }
  if (!SetEvent(mCommandLoopCtx->PushEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mCommandLoopCtx->IsIdle = false;

  return S_OK;
}

STDMETHODIMP CAudioServer::GetVoiceCount(INT32 *pVoiceCount) {
  if (pVoiceCount == nullptr || mVoiceInfoCtx == nullptr) {
    return E_FAIL;
  }

  Log->Info(L"Called IAudioServer::GetVoiceCount()", GetCurrentThreadId(),
            __LONGFILE__);

  *pVoiceCount = mVoiceInfoCtx->Count;

  return S_OK;
}

STDMETHODIMP CAudioServer::GetDefaultVoice(INT32 *pVoiceIndex) {
  if (pVoiceIndex == nullptr || mVoiceInfoCtx == nullptr) {
    return E_FAIL;
  }

  Log->Info(L"Called IAudioServer::GetDefaultVoice", GetCurrentThreadId(),
            __LONGFILE__);

  *pVoiceIndex = mVoiceInfoCtx->DefaultVoiceIndex;

  return S_OK;
}

STDMETHODIMP
CAudioServer::GetVoiceProperty(INT32 index,
                               RawVoiceProperty **ppRawVoiceProperty) {
  if (mVoiceInfoCtx == nullptr || mVoiceInfoCtx->VoiceProperties == nullptr ||
      index < 0 || index > (mVoiceInfoCtx->Count - 1)) {
    return E_FAIL;
  }

  Log->Info(L"Called IAudioServer::GetVoiceProperty()", GetCurrentThreadId(),
            __LONGFILE__);

  (*ppRawVoiceProperty) = reinterpret_cast<RawVoiceProperty *>(
      CoTaskMemAlloc(sizeof(RawVoiceProperty)));
  (*ppRawVoiceProperty)->Language =
      reinterpret_cast<LPWSTR>(CoTaskMemAlloc(128));
  (*ppRawVoiceProperty)->DisplayName =
      reinterpret_cast<LPWSTR>(CoTaskMemAlloc(128));

  HRESULT hr{};

  hr = StringCbPrintfW((*ppRawVoiceProperty)->Language, 128, L"%s",
                       mVoiceInfoCtx->VoiceProperties[index]->Language);

  if (FAILED(hr)) {
    return hr;
  }

  hr = StringCbPrintfW((*ppRawVoiceProperty)->DisplayName, 128, L"%s",
                       mVoiceInfoCtx->VoiceProperties[index]->DisplayName);

  if (FAILED(hr)) {
    return hr;
  }

  (*ppRawVoiceProperty)->SpeakingRate =
      mVoiceInfoCtx->VoiceProperties[index]->SpeakingRate;
  (*ppRawVoiceProperty)->Pitch =
      mVoiceInfoCtx->VoiceProperties[index]->AudioPitch;
  (*ppRawVoiceProperty)->Volume =
      mVoiceInfoCtx->VoiceProperties[index]->AudioVolume;

  return S_OK;
}

STDMETHODIMP CAudioServer::SetDefaultVoice(INT32 index) {
  if (mVoiceInfoCtx == nullptr || index < 0 ||
      index > static_cast<int32_t>(mVoiceInfoCtx->Count)) {
    return E_FAIL;
  }

  Log->Info(L"Called IAudioServer::SetDefaultVoice", GetCurrentThreadId(),
            __LONGFILE__);

  mVoiceInfoCtx->DefaultVoiceIndex = index;

  return S_OK;
}

STDMETHODIMP
CAudioServer::SetVoiceProperty(INT32 index,
                               RawVoiceProperty *pRawVoiceProperty) {
  if (mVoiceInfoCtx == nullptr || index < 0 ||
      index > (mVoiceInfoCtx->Count - 1) ||
      mVoiceInfoCtx->VoiceProperties == nullptr) {
    return E_FAIL;
  }

  Log->Info(L"Called IAudioServer::SetVoiceProperty", GetCurrentThreadId(),
            __LONGFILE__);

  if (pRawVoiceProperty->SpeakingRate >= 0.0) {
    double rate = pRawVoiceProperty->SpeakingRate;

    if (rate < 0.5) {
      rate = 0.5;
    }
    if (rate > 6.0) {
      rate = 6.0;
    }

    mVoiceInfoCtx->VoiceProperties[index]->SpeakingRate = rate;
  }
  if (pRawVoiceProperty->Pitch >= 0.0) {
    double pitch = pRawVoiceProperty->Pitch;

    if (pitch > 2.0) {
      pitch = 2.0;
    }

    mVoiceInfoCtx->VoiceProperties[index]->AudioPitch = pitch;
  }
  if (pRawVoiceProperty->Volume >= 0.0) {
    double volume = pRawVoiceProperty->Volume;

    if (volume > 1.0) {
      volume = 1.0;
    }

    mVoiceInfoCtx->VoiceProperties[index]->AudioVolume = volume;
  }

  return S_OK;
}

STDMETHODIMP
CAudioServer::SetNotifyIdleStateHandler(
    NotifyIdleStateHandler notifyIdleStateHandler) {
  if (mCommandLoopCtx == nullptr) {
    return E_FAIL;
  }

  mCommandLoopCtx->NotifyIdleState = notifyIdleStateHandler;

  return S_OK;
}

// CAudioServerFactory
STDMETHODIMP CAudioServerFactory::QueryInterface(REFIID riid,
                                                 void **ppvObject) {
  *ppvObject = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
    *ppvObject = static_cast<IClassFactory *>(this);
  } else {
    return E_NOINTERFACE;
  }

  AddRef();

  return S_OK;
}

STDMETHODIMP_(ULONG) CAudioServerFactory::AddRef() {
  LockModule(true);

  return 2;
}

STDMETHODIMP_(ULONG) CAudioServerFactory::Release() {
  LockModule(false);

  return 1;
}

STDMETHODIMP CAudioServerFactory::CreateInstance(IUnknown *pUnkOuter,
                                                 REFIID riid,
                                                 void **ppvObject) {
  CAudioServer *p{};
  HRESULT hr{};

  *ppvObject = nullptr;

  if (pUnkOuter != nullptr) {
    return CLASS_E_NOAGGREGATION;
  }

  p = new CAudioServer();

  if (p == nullptr) {
    return E_OUTOFMEMORY;
  }

  hr = p->QueryInterface(riid, ppvObject);
  p->Release();

  return hr;
}

STDMETHODIMP CAudioServerFactory::LockServer(BOOL fLock) {
  LockModule(fLock);

  return S_OK;
}

// DLL Export
STDAPI DllCanUnloadNow(void) { return LockCount == 0 ? S_OK : S_FALSE; }

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
  static CAudioServerFactory serverFactory;
  HRESULT hr{};
  *ppv = nullptr;

  if (IsEqualCLSID(rclsid, CLSID_AudioServer)) {
    hr = serverFactory.QueryInterface(riid, ppv);
  } else {
    hr = CLASS_E_CLASSNOTAVAILABLE;
  }

  return hr;
}

STDAPI DllRegisterServer(void) {
  TCHAR szModulePath[256]{};
  TCHAR szKey[256]{};
  WCHAR szTypeLibPath[256]{};
  HRESULT hr{};
  ITypeLib *pTypeLib{};

  wsprintf(szKey, TEXT("CLSID\\%s"), AudioServerCLSIDStr);

  if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, nullptr,
                         TEXT("COM Audio Server"))) {
    return E_FAIL;
  }

  GetModuleFileName(AudioServerDLLInstance, szModulePath,
                    sizeof(szModulePath) / sizeof(TCHAR));
  wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), AudioServerCLSIDStr);

  if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, nullptr, szModulePath)) {
    return E_FAIL;
  }

  wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), AudioServerCLSIDStr);

  if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, TEXT("ThreadingModel"),
                         TEXT("Apartment"))) {
    return E_FAIL;
  }

  wsprintf(szKey, TEXT("CLSID\\%s\\ProgID"), AudioServerCLSIDStr);

  if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, nullptr,
                         (LPTSTR)ProgIDStr)) {
    return E_FAIL;
  }

  wsprintf(szKey, TEXT("%s"), ProgIDStr);

  if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, nullptr,
                         TEXT("COM Audio Server"))) {
    return E_FAIL;
  }

  wsprintf(szKey, TEXT("%s\\CLSID"), ProgIDStr);

  if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, nullptr,
                         (LPTSTR)AudioServerCLSIDStr)) {
    return E_FAIL;
  }

  GetModuleFileNameW(AudioServerDLLInstance, szTypeLibPath,
                     sizeof(szTypeLibPath) / sizeof(TCHAR));

  hr = LoadTypeLib(szTypeLibPath, &pTypeLib);

  if (SUCCEEDED(hr)) {
    hr = RegisterTypeLib(pTypeLib, szTypeLibPath, nullptr);

    if (SUCCEEDED(hr)) {
      wsprintf(szKey, TEXT("CLSID\\%s\\TypeLib"), AudioServerCLSIDStr);

      if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, nullptr, LibraryIDStr)) {
        pTypeLib->Release();

        return E_FAIL;
      }
    }

    pTypeLib->Release();
  }

  return S_OK;
}

STDAPI DllUnregisterServer(void) {
  TCHAR szKey[256]{};

  wsprintf(szKey, TEXT("CLSID\\%s"), AudioServerCLSIDStr);
  SHDeleteKey(HKEY_CLASSES_ROOT, szKey);

  wsprintf(szKey, TEXT("%s"), ProgIDStr);
  SHDeleteKey(HKEY_CLASSES_ROOT, szKey);

  UnRegisterTypeLib(LIBID_AudioServerLib, 1, 0, LOCALE_NEUTRAL, SYS_WIN32);

  return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    AudioServerDLLInstance = hInstance;
    DisableThreadLibraryCalls(hInstance);
    GetGuidString(CLSID_AudioServer, AudioServerCLSIDStr);
    GetGuidString(LIBID_AudioServerLib, LibraryIDStr);

    return TRUE;
  }

  return TRUE;
}

// Helper function
void LockModule(BOOL bLock) {
  if (bLock) {
    InterlockedIncrement(&LockCount);
  } else {
    InterlockedDecrement(&LockCount);
  }
}

BOOL CreateRegistryKey(HKEY hKeyRoot, LPTSTR lpszKey, LPTSTR lpszValue,
                       LPTSTR lpszData) {
  HKEY hKey{};
  LONG lResult{};
  DWORD dwSize{};

  lResult =
      RegCreateKeyEx(hKeyRoot, lpszKey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                     KEY_WRITE, nullptr, &hKey, nullptr);

  if (lResult != ERROR_SUCCESS) {
    return FALSE;
  }
  if (lpszData != nullptr) {
    dwSize = (lstrlen(lpszData) + 1) * sizeof(TCHAR);
  } else {
    dwSize = 0;
  }

  RegSetValueEx(hKey, lpszValue, 0, REG_SZ, (LPBYTE)lpszData, dwSize);
  RegCloseKey(hKey);

  return TRUE;
}

void GetGuidString(REFGUID rguid, LPTSTR lpszGuid) {
  LPWSTR lpsz;
  StringFromCLSID(rguid, &lpsz);

#ifdef UNICODE
  lstrcpyW(lpszGuid, lpsz);
#else
  WideCharToMultiByte(CP_ACP, 0, lpsz, -1, lpszGuid, 256, nullptr, nullptr);
#endif

  CoTaskMemFree(lpsz);
}
