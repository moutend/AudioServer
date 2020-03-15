#pragma once

#include <functional>
#include <robuffer.h>
#include <windows.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace Windows::Storage::Streams;

template <class T> void SafeRelease(T **ppT) {
  if (*ppT) {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

void SafeCloseHandle(HANDLE *pHandle);
char *getBytes(IBuffer ^ buffer);
