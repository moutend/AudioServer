#include <cpplogger/cpplogger.h>
#include <cstring>
#include <fstream>
#include <mutex>
#include <shlwapi.h>
#include <strsafe.h>
#include <windows.h>

#include <strsafe.h>

#include "AudioServer.h"
#include <IAudioServer/IAudioServer.h>

extern Logger::Logger *Log;

const TCHAR ProgIDStr[] = TEXT("ScreenReaderX.AudioServer");
LONG LockCount{};
HINSTANCE AudioServerDLLInstance{};
TCHAR AudioServerCLSIDStr[256]{};
TCHAR LibraryIDStr[256]{};

// CAudioServer
CAudioServer::CAudioServer()
    : mReferenceCount{1}, mTypeInfo{}, mMaxWaves{100}, mIsActive{},
      mLoggingCtx{}, mCommandCtx{}, mVoiceInfoCtx{}, mVoiceCtx{}, mSFXCtx{},
      mRenderCtx{}, mLoggingThread{}, mCommandThread{}, mVoiceInfoThread{},
      mVoiceThread{}, mSFXThread{}, mRenderThread{}, mNextEvent{}, mEngine{} {
  ITypeLib *pTypeLib{};
  HRESULT hr{};

  LockModule(true);

  hr = LoadRegTypeLib(LIBID_AudioServerLib, 1, 0, 0, &pTypeLib);

  if (SUCCEEDED(hr)) {
    pTypeLib->GetTypeInfoOfGuid(IID_IAudioServer, &mTypeInfo);
    pTypeLib->Release();
  }

  mVoiceInfoCtx = new VoiceInfoContext;

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

STDMETHODIMP CAudioServer::Start(LPWSTR soundEffectsPath, LPWSTR loggerURL,
                                 LOGLEVEL level) {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (mIsActive) {
    Log->Warn(L"Already initialized", GetCurrentThreadId(), __LONGFILE__);
    return S_OK;
  }

  mIsActive = true;

  Log = new Logger::Logger(L"AudioServer", L"v0.1.0-develop", 8192);

  Log->Info(L"Setup audio server", GetCurrentThreadId(), __LONGFILE__);

  mLoggingCtx = new LoggingContext;

  mLoggingCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mLoggingCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create logging thread", GetCurrentThreadId(), __LONGFILE__);

  mLoggingThread = CreateThread(nullptr, 0, loggingThread,
                                static_cast<void *>(mLoggingCtx), 0, nullptr);

  if (mLoggingThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mNextEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mNextEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mEngine = new PCMAudio::KickEngine(mMaxWaves, 32);

  for (int16_t i = 0; i < mMaxWaves; i++) {
    wchar_t *filePath = new wchar_t[256]{};

    HRESULT hr = StringCbPrintfW(filePath, 512, L"%s\\%03d.wav",
                                 soundEffectsPath, i + 1);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to build file path", GetCurrentThreadId(),
                __LONGFILE__);
      continue;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::in);

    if (!file.is_open()) {
      Log->Fail(L"Failed to open file", GetCurrentThreadId(), __LONGFILE__);
    }

    mEngine->Register(i, file);

    Log->Info(filePath, GetCurrentThreadId(), __LONGFILE__);

    delete[] filePath;
    filePath = nullptr;

    file.close();
  }

  mRenderCtx = new RenderContext;
  mRenderCtx->NextEvent = mNextEvent;
  mRenderCtx->Engine = mEngine;

  mRenderCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mRenderCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create render thread", GetCurrentThreadId(), __LONGFILE__);

  mRenderThread = CreateThread(nullptr, 0, renderThread,
                               static_cast<void *>(mRenderCtx), 0, nullptr);

  if (mRenderThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mVoiceCtx = new VoiceContext;
  mVoiceCtx->Engine = mEngine;
  mVoiceCtx->VoiceInfoCtx = mVoiceInfoCtx;
  mVoiceCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mVoiceCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mVoiceCtx->KickEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mVoiceCtx->KickEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create voice thread", GetCurrentThreadId(), __LONGFILE__);

  mVoiceThread = CreateThread(nullptr, 0, voiceThread,
                              static_cast<void *>(mVoiceCtx), 0, nullptr);

  if (mVoiceThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mSFXCtx = new SFXContext;
  mSFXCtx->Engine = mEngine;

  mSFXCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mSFXCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mSFXCtx->KickEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mSFXCtx->KickEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create sfx thread", GetCurrentThreadId(), __LONGFILE__);

  mSFXThread = CreateThread(nullptr, 0, sfxThread, static_cast<void *>(mSFXCtx),
                            0, nullptr);

  if (mSFXThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mCommandCtx = new CommandContext;
  mCommandCtx->NextEvent = mNextEvent;
  mCommandCtx->VoiceCtx = mVoiceCtx;
  mCommandCtx->SFXCtx = mSFXCtx;
  mCommandCtx->Commands = new Command *[mCommandCtx->MaxCommands] {};

  for (int32_t i = 0; i < mCommandCtx->MaxCommands; i++) {
    mCommandCtx->Commands[i] = new Command;
    mCommandCtx->Commands[i]->Type = 0;
    mCommandCtx->Commands[i]->SFXIndex = 0;
    mCommandCtx->Commands[i]->SleepDuration = 0;
    mCommandCtx->Commands[i]->Text = nullptr;
  }

  mCommandCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mCommandCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mCommandCtx->PushEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (mCommandCtx->PushEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"Create command thread", GetCurrentThreadId(), __LONGFILE__);

  mCommandThread = CreateThread(nullptr, 0, commandThread,
                                static_cast<void *>(mCommandCtx), 0, nullptr);

  if (mCommandThread == nullptr) {
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

  if (mCommandThread == nullptr) {
    goto END_COMMAND_CLEANUP;
  }
  if (!SetEvent(mCommandCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mCommandThread, INFINITE);
  SafeCloseHandle(&mCommandThread);

  for (int32_t i = 0; i < mCommandCtx->MaxCommands; i++) {
    delete[] mCommandCtx->Commands[i]->Text;
    mCommandCtx->Commands[i]->Text = nullptr;

    delete mCommandCtx->Commands[i];
    mCommandCtx->Commands[i] = nullptr;
  }

  SafeCloseHandle(&(mCommandCtx->QuitEvent));
  SafeCloseHandle(&(mCommandCtx->PushEvent));

  delete mCommandCtx;
  mCommandCtx = nullptr;

  Log->Info(L"Delete command thread", GetCurrentThreadId(), __LONGFILE__);

END_COMMAND_CLEANUP:

  if (mVoiceThread == nullptr) {
    goto END_VOICE_CLEANUP;
  }
  if (!SetEvent(mVoiceCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mVoiceThread, INFINITE);
  SafeCloseHandle(&mVoiceThread);

  SafeCloseHandle(&(mVoiceCtx->QuitEvent));
  SafeCloseHandle(&(mVoiceCtx->KickEvent));

  delete mVoiceCtx;
  mVoiceCtx = nullptr;

  Log->Info(L"Delete voice thread", GetCurrentThreadId(), __LONGFILE__);

END_VOICE_CLEANUP:

  if (mSFXThread == nullptr) {
    goto END_SFX_CLEANUP;
  }
  if (!SetEvent(mSFXCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mSFXThread, INFINITE);
  SafeCloseHandle(&mSFXThread);

  SafeCloseHandle(&(mSFXCtx->QuitEvent));
  SafeCloseHandle(&(mSFXCtx->KickEvent));

  delete mSFXCtx;
  mSFXCtx = nullptr;

  Log->Info(L"Delete SFX thread", GetCurrentThreadId(), __LONGFILE__);

END_SFX_CLEANUP:

  if (mRenderThread == nullptr) {
    goto END_RENDER_CLEANUP;
  }
  if (!SetEvent(mRenderCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mRenderThread, INFINITE);
  SafeCloseHandle(&mRenderThread);

  SafeCloseHandle(&(mRenderCtx->QuitEvent));
  SafeCloseHandle(&mNextEvent);

  delete mEngine;
  mEngine = nullptr;

  Log->Info(L"Delete render thread", GetCurrentThreadId(), __LONGFILE__);

END_RENDER_CLEANUP:

  Log->Info(L"Complete Stop audio server", GetCurrentThreadId(), __LONGFILE__);

  if (mLoggingThread == nullptr) {
    goto END_LOGGING_CLEANUP;
  }
  if (!SetEvent(mLoggingCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  WaitForSingleObject(mLoggingThread, INFINITE);
  SafeCloseHandle(&mLoggingThread);

  SafeCloseHandle(&(mLoggingCtx->QuitEvent));

  delete mLoggingCtx;
  mLoggingCtx = nullptr;

END_LOGGING_CLEANUP:

  mIsActive = false;

  return S_OK;
}

STDMETHODIMP CAudioServer::Restart() {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (!mIsActive) {
    return E_FAIL;
  }

  Log->Info(L"Called Restart()", GetCurrentThreadId(), __LONGFILE__);

  mEngine->Restart();

  return S_OK;
}

STDMETHODIMP CAudioServer::Pause() {
  std::lock_guard<std::mutex> lock(mAudioServerMutex);

  if (!mIsActive) {
    return E_FAIL;
  }

  Log->Info(L"Called Pause()", GetCurrentThreadId(), __LONGFILE__);

  mEngine->Pause();

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

  wchar_t *buffer = new wchar_t[256]{};

  HRESULT hr =
      StringCbPrintfW(buffer, 512,
                      L"Called IAudioServer::Push() "
                      L"Read=%d,Write=%d,Length=%d,IsForce=%d,Wait=%.1f",
                      mCommandCtx->ReadIndex, mCommandCtx->WriteIndex,
                      commandsLength, isForcePush, pCommands[0]->SleepDuration);

  if (FAILED(hr)) {
    return E_FAIL;
  }

  Log->Info(buffer, GetCurrentThreadId(), __LONGFILE__);

  delete[] buffer;
  buffer = nullptr;

  bool isIdle = mCommandCtx->IsIdle;
  int32_t base = mCommandCtx->WriteIndex;
  size_t textLen{};

  for (int32_t i = 0; i < commandsLength; i++) {
    int32_t offset = (base + i) % mCommandCtx->MaxCommands;

    mCommandCtx->Commands[offset]->Type = pCommands[i]->Type;

    switch (pCommands[i]->Type) {
    case 1:
      mCommandCtx->Commands[offset]->Pan = pCommands[i]->Pan;
      mCommandCtx->Commands[offset]->SFXIndex =
          pCommands[i]->SFXIndex <= 0 ? 0 : pCommands[i]->SFXIndex - 1;
      break;
    case 2:
      mCommandCtx->Commands[offset]->SleepDuration =
          pCommands[i]->SleepDuration;
      break;
    case 3: // Generate voice from plain text
    case 4: // Generate voice from SSML
      delete[] mCommandCtx->Commands[offset]->Text;
      mCommandCtx->Commands[offset]->Text = nullptr;

      textLen = std::wcslen(pCommands[i]->TextToSpeech);
      mCommandCtx->Commands[offset]->Text = new wchar_t[textLen + 1]{};
      std::wmemcpy(mCommandCtx->Commands[offset]->Text,
                   pCommands[i]->TextToSpeech, textLen);

      break;
    default:
      // do nothing
      continue;
    }

    mCommandCtx->WriteIndex =
        (mCommandCtx->WriteIndex + 1) % mCommandCtx->MaxCommands;
  }
  if (isForcePush) {
    mCommandCtx->ReadIndex = base;
  }
  if (!isForcePush && !isIdle) {
    return S_OK;
  }
  if (!SetEvent(mCommandCtx->PushEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  mCommandCtx->IsIdle = false;

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
  {
    wchar_t *buffer = new wchar_t[256]{};
    HRESULT hr =
        StringCbPrintfW(buffer, 512,
                        L"Called IAudioServer::SetVoiceProperty() "
                        L"index=%d,speakingRate=%.1f,pitch=%.1f,volume=%.1f",
                        index, pRawVoiceProperty->SpeakingRate,
                        pRawVoiceProperty->Pitch, pRawVoiceProperty->Volume);

    if (FAILED(hr)) {
      return hr;
    }

    Log->Info(buffer, GetCurrentThreadId(), __LONGFILE__);

    delete[] buffer;
    buffer = nullptr;
  }
  {
    double rate = pRawVoiceProperty->SpeakingRate;

    if (rate < 0.5) {
      rate = 0.5;
    }
    if (rate > 6.0) {
      rate = 6.0;
    }

    mVoiceInfoCtx->VoiceProperties[index]->SpeakingRate = rate;
  }
  {
    double pitch = pRawVoiceProperty->Pitch;

    if (pitch > 2.0) {
      pitch = 2.0;
    }

    mVoiceInfoCtx->VoiceProperties[index]->AudioPitch = pitch;
  }
  {
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
  if (mCommandCtx == nullptr) {
    return E_FAIL;
  }

  mCommandCtx->NotifyIdleState = notifyIdleStateHandler;

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
                         TEXT("ScreenReaderX.AudioServer"))) {
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
                         TEXT("ScreenReaderX.AudioServer"))) {
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
