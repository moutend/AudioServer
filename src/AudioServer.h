#pragma once

#include <IAudioServer/IAudioServer.h>
#include <cppaudio/engine.h>
#include <mutex>
#include <windows.h>

#include "audiothread.h"
#include "commandthread.h"
#include "context.h"
#include "loggingthread.h"
#include "sfxthread.h"
#include "util.h"
#include "voiceinfo.h"
#include "voicethread.h"

class CAudioServer : public IAudioServer {
public:
  // IUnknown methods
  STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IDispatch methods
  STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
  STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
  STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
                             LCID lcid, DISPID *rgDispId);
  STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                      EXCEPINFO *pExcepInfo, UINT *puArgErr);

  // IAudioServer methods
  STDMETHODIMP Start(LPWSTR soundEffectsPath, LPWSTR loggerURL, LOGLEVEL level);
  STDMETHODIMP Stop();
  STDMETHODIMP Restart();
  STDMETHODIMP Pause();
  STDMETHODIMP Push(RawCommand **pCommands, INT32 commandsLength,
                    INT32 isForcePush);
  STDMETHODIMP GetVoiceCount(INT32 *pVoiceCount);
  STDMETHODIMP GetDefaultVoice(INT32 *pVoiceIndex);
  STDMETHODIMP GetVoiceProperty(INT32 index,
                                RawVoiceProperty **ppRawVoiceProperty);
  STDMETHODIMP SetDefaultVoice(INT32 index);
  STDMETHODIMP SetVoiceProperty(INT32 index, RawVoiceProperty *pVoiceProperty);
  STDMETHODIMP
  SetNotifyIdleStateHandler(NotifyIdleStateHandler notifyStateHandler);

  CAudioServer();
  ~CAudioServer();

private:
  LONG mReferenceCount;
  ITypeInfo *mTypeInfo;

  int16_t mMaxWaves = 128;
  bool mIsActive = false;
  std::mutex mAudioServerMutex;

  LoggingContext *mLoggingCtx = nullptr;
  CommandContext *mCommandCtx = nullptr;
  VoiceInfoContext *mVoiceInfoCtx = nullptr;
  VoiceContext *mVoiceCtx = nullptr;
  SFXContext *mSFXCtx = nullptr;
  AudioContext *mRenderCtx = nullptr;

  HANDLE mLoggingThread = nullptr;
  HANDLE mCommandThread = nullptr;
  HANDLE mVoiceInfoThread = nullptr;
  HANDLE mVoiceThread = nullptr;
  HANDLE mSFXThread = nullptr;
  HANDLE mRenderThread = nullptr;

  HANDLE mNextEvent = nullptr;

  PCMAudio::KickEngine *mEngine = nullptr;
};

class CAudioServerFactory : public IClassFactory {
public:
  // IUnknown methods
  STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IClassFactory methods
  STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                              void **ppvObject);
  STDMETHODIMP LockServer(BOOL fLock);
};

extern void LockModule(BOOL bLock);
extern BOOL CreateRegistryKey(HKEY hKeyRoot, LPTSTR lpszKey, LPTSTR lpszValue,
                              LPTSTR lpszData);
extern void GetGuidString(REFGUID rguid, LPTSTR lpszGuid);
