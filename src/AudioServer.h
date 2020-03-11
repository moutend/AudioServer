#pragma once

#include <windows.h>
#include <AudioServer/IAudioServer.h>

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
  STDMETHODIMP Push(AudioCommand **pAudioCommands, INT32 numAudioCommands);
  STDMETHODIMP GetVoiceCount(INT32 *pVoiceCount);
  STDMETHODIMP GetDefaultVoice(INT32 *pVoiceIndex);
  STDMETHODIMP GetVoiceProperty(INT32 voiceIndex, VoiceProperty **pVoiceProperty);
  STDMETHODIMP SetDefaultVoice(INT32 voiceIndex);
  STDMETHODIMP SetVoiceProperty(INT32 voiceIndex, VoiceProperty *pVoiceProperty);

  CAudioServer();
  ~CAudioServer();

private:
  LONG mReferenceCount;
  ITypeInfo *mTypeInfo;
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
