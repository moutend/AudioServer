#pragma once

#include <AudioClient.h>
#include <AudioPolicy.h>
#include <MMDeviceAPI.h>
#include <windows.h>

class Notification : public IMMNotificationClient {
public:
  Notification(HANDLE streamSwitchEvent, ERole deviceRole, wchar_t *deviceId);
  ~Notification();

  // IMMNotificationClient
  STDMETHOD(OnDeviceStateChanged)(LPCWSTR /*DeviceId*/, DWORD /*NewState*/);
  STDMETHOD(OnDeviceAdded)(LPCWSTR /*DeviceId*/) { return S_OK; };
  STDMETHOD(OnDeviceRemoved)(LPCWSTR /*DeviceId(*/) { return S_OK; };
  STDMETHOD(OnDefaultDeviceChanged)
  (EDataFlow flow, ERole role, LPCWSTR NewDefaultDeviceId);
  STDMETHOD(OnPropertyValueChanged)
  (LPCWSTR /*DeviceId*/, const PROPERTYKEY /*Key*/) { return S_OK; };

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID iid, void **pvObject);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

private:
  LONG mRefCount = 1;
  bool mInStreamSwitch;
  ERole mDeviceRole;

  wchar_t mDeviceId[512]{};
  HANDLE mStreamSwitchEvent = nullptr;
  HANDLE mStreamSwitchCompleteEvent = nullptr;
};
