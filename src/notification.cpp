#include <cpplogger/cpplogger.h>
#include <cstring>
#include <mutex>
#include <sstream>

#include "notification.h"
#include "util.h"

using namespace Windows::Media::Devices;

extern Logger::Logger *Log;

std::mutex notificationMutex;

Notification::Notification(HANDLE streamSwitchEvent, ERole deviceRole,
                           wchar_t *deviceId)
    : mStreamSwitchEvent(streamSwitchEvent), mDeviceRole(deviceRole) {
  std::wmemcpy(mDeviceId, deviceId, 256);
}

Notification::~Notification() {}

HRESULT Notification::OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                             LPCWSTR newDefaultDeviceId) {
  if (flow != eRender) {
    return S_OK;
  }

  std::lock_guard<std::mutex> lock(notificationMutex);

  bool isDeviceChanged = (std::wcscmp(newDefaultDeviceId, mDeviceId) != 0);
  std::wstringstream wss;

  wss << L"New default device: " << newDefaultDeviceId << L" / ";
  // wss << L"Current default device" << mDeviceId;

  Log->Info(wss.str(), GetCurrentThreadId(), __LONGFILE__);

  if (isDeviceChanged && mStreamSwitchEvent != nullptr) {
    Log->Info(L"Send event (mStreamSwitchEvent)", GetCurrentThreadId(),
              __LONGFILE__);

    if (!SetEvent(mStreamSwitchEvent)) {
      Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    }
    mStreamSwitchEvent = nullptr;
  }

  return S_OK;
}

HRESULT Notification::OnDeviceStateChanged(LPCWSTR deviceId, DWORD newState) {
  std::lock_guard<std::mutex> lock(notificationMutex);

  if (newState == DEVICE_STATE_ACTIVE && mStreamSwitchEvent != nullptr) {
    if (!SetEvent(mStreamSwitchEvent)) {
      Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    }
  }

  return S_OK;
}

HRESULT Notification::QueryInterface(REFIID Iid, void **Object) {
  if (Object == nullptr) {
    return E_POINTER;
  }
  *Object = nullptr;

  if (Iid == __uuidof(IMMNotificationClient)) {
    *Object = static_cast<IMMNotificationClient *>(this);
    AddRef();
  } else {
    return E_NOINTERFACE;
  }

  return S_OK;
}

ULONG Notification::AddRef() { return InterlockedIncrement(&mRefCount); }

ULONG Notification::Release() {
  ULONG returnValue = InterlockedDecrement(&mRefCount);

  if (returnValue == 0) {
    delete this;
  }

  return returnValue;
}
