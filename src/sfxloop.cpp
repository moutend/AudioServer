#include <cpplogger/cpplogger.h>
#include <strsafe.h>

#include "context.h"
#include "sfxloop.h"
#include "util.h"

extern Logger::Logger *Log;

DWORD WINAPI sfxLoop(LPVOID context) {
  Log->Info(L"Start SFX loop thread", GetCurrentThreadId(), __LONGFILE__);

  SFXLoopContext *ctx = static_cast<SFXLoopContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain ctx", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  bool isActive{true};

  while (isActive) {
    HANDLE waitArray[2] = {ctx->QuitEvent, ctx->FeedEvent};
    DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);

    // Check ctx->QuitEvent
    if (waitResult == WAIT_OBJECT_0 + 0) {
      break;
    }
    if (ctx->SFXIndex < 0) {
      wchar_t *buffer = new wchar_t[128]{};
      StringCbPrintfW(buffer, 256, L"@@@wait %.1f", ctx->WaitDuration);

      Log->Info(buffer, GetCurrentThreadId(), __LONGFILE__);

      delete[] buffer;
      buffer = nullptr;

      ctx->SFXEngine->Sleep(ctx->WaitDuration);
    } else {
      ctx->SFXEngine->Start(ctx->SFXIndex);
    }
  }

  Log->Info(L"End SFX loop thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
