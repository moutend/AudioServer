#include "util.h"

#include <strsafe.h>

// These headers must be included after windows.h.
#include <Knownfolders.h>
#include <ShlObj_core.h>
#include <fileapi.h>

void SafeCloseHandle(HANDLE *pHandle) {
  if (pHandle) {
    CloseHandle(*pHandle);
    *pHandle = nullptr;
  }
}

char *getBytes(IBuffer ^ buffer) {
  ComPtr<IInspectable> i = reinterpret_cast<IInspectable *>(buffer);
  ComPtr<IBufferByteAccess> bufferByteAccess;

  if (FAILED(i.As(&bufferByteAccess))) {
    return nullptr;
  }

  byte *bs{nullptr};
  bufferByteAccess->Buffer(&bs);

  return reinterpret_cast<char *>(bs);
}
