#include <cpplogger/cpplogger.h>
#include <strsafe.h>

#include "context.h"
#include "sfxthread.h"
#include "util.h"

extern Logger::Logger *Log;

DWORD WINAPI sfxThread(LPVOID context) {
  Log->Info(L"Start SFX thread", GetCurrentThreadId(), __LONGFILE__);

  SFXContext *ctx = static_cast<SFXContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to get context", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  bool isActive{true};

  while (isActive) {
    HANDLE waitArray[2] = {ctx->QuitEvent, ctx->KickEvent};
    DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);

    // Check ctx->QuitEvent
    if (waitResult == WAIT_OBJECT_0 + 0) {
      break;
    }
    if (ctx->SFXIndex < 0) {
      ctx->Engine->Sleep(ctx->SleepDuration);
    } else {
      ctx->Engine->Kick(ctx->SFXIndex, ctx->Pan);
    }
  }

  Log->Info(L"End SFX thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
