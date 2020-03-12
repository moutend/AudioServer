#include <shlwapi.h>
#include <windows.h>

#include "AudioServer.h"
#include "AudioServer/IAudioServer.h"

const TCHAR ProgIDStr[] = TEXT("Sample.AudioServer");
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
}

CAudioServer::~CAudioServer() { LockModule(false); }

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

STDMETHODIMP CAudioServer::Start() { return E_NOTIMPL; }

STDMETHODIMP CAudioServer::Stop() { return E_NOTIMPL; }

STDMETHODIMP CAudioServer::FadeIn() { return E_NOTIMPL; }

STDMETHODIMP CAudioServer::FadeOut() { return E_NOTIMPL; }

STDMETHODIMP CAudioServer::Push(AudioCommand **pAudioCommands,
                                INT32 numAudioCommands) {
  return E_NOTIMPL;
}

STDMETHODIMP CAudioServer::GetVoiceCount(INT32 *pVoiceCount) {
  return E_NOTIMPL;
}

STDMETHODIMP CAudioServer::GetDefaultVoice(INT32 *pVoiceCount) {
  return E_NOTIMPL;
}

STDMETHODIMP CAudioServer::GetVoiceProperty(INT32 voiceIndex,
                                            VoiceProperty **pVoiceProperty) {
  return E_NOTIMPL;
}

STDMETHODIMP CAudioServer::SetDefaultVoice(INT32 voiceIndex) {
  return E_NOTIMPL;
}

STDMETHODIMP CAudioServer::SetVoiceProperty(INT32 voiceIndex,
                                            VoiceProperty *pVoiceProperty) {
  return E_NOTIMPL;
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
                         TEXT("COM Server Sample2"))) {
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
                         TEXT("COM Server Sample2"))) {
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