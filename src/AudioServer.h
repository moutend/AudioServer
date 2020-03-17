#pragma once

#include <AudioServer/IAudioServer.h>
#include <cppaudio/engine.h>
#include <mutex>
#include <windows.h>

#include "audioloop.h"
#include "commandloop.h"
#include "context.h"
#include "logloop.h"
#include "sfxloop.h"
#include "util.h"
#include "voiceinfo.h"
#include "voiceloop.h"

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
  STDMETHODIMP Start();
  STDMETHODIMP Stop();
  STDMETHODIMP FadeIn();
  STDMETHODIMP FadeOut();
  STDMETHODIMP Push(RawCommand **pCommands, INT32 commandsLength,
                    INT32 isForcePush);
  STDMETHODIMP GetVoiceCount(INT32 *pVoiceCount);
  STDMETHODIMP GetDefaultVoice(INT32 *pVoiceIndex);
  STDMETHODIMP GetVoiceProperty(INT32 index,
                                RawVoiceProperty **ppVoiceProperty);
  STDMETHODIMP SetDefaultVoice(INT32 index);
  STDMETHODIMP SetVoiceProperty(INT32 index, RawVoiceProperty *pVoiceProperty);

  CAudioServer();
  ~CAudioServer();

private:
  LONG mReferenceCount;
  ITypeInfo *mTypeInfo;

  int16_t mMaxWaves = 128;
  bool mIsActive = false;
  std::mutex mAudioServerMutex;

  LogLoopContext *mLogLoopCtx = nullptr;
  CommandLoopContext *mCommandLoopCtx = nullptr;
  VoiceInfoContext *mVoiceInfoCtx = nullptr;
  VoiceLoopContext *mVoiceLoopCtx = nullptr;
  SFXLoopContext *mSFXLoopCtx = nullptr;
  AudioLoopContext *mVoiceRenderCtx = nullptr;
  AudioLoopContext *mSFXRenderCtx = nullptr;

  HANDLE mLogLoopThread = nullptr;
  HANDLE mCommandLoopThread = nullptr;
  HANDLE mVoiceInfoThread = nullptr;
  HANDLE mVoiceLoopThread = nullptr;
  HANDLE mVoiceRenderThread = nullptr;
  HANDLE mSFXLoopThread = nullptr;
  HANDLE mSFXRenderThread = nullptr;

  HANDLE mNextVoiceEvent = nullptr;
  HANDLE mNextSoundEvent = nullptr;

  PCMAudio::RingEngine *mVoiceEngine = nullptr;
  PCMAudio::LauncherEngine *mSFXEngine = nullptr;
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
