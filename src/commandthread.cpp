#include <cpplogger/cpplogger.h>
#include <windows.h>

#include "commandthread.h"
#include "context.h"
#include "util.h"

#include <strsafe.h>

extern Logger::Logger *Log;

DWORD WINAPI commandThread(LPVOID context) {
  Log->Info(L"Start command thread", GetCurrentThreadId(), __LONGFILE__);

  CommandContext *ctx = static_cast<CommandContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to get context", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  bool isActive{true};
  Command *cmd{};

  while (isActive) {
    HANDLE waitArray[3] = {ctx->QuitEvent, ctx->PushEvent, ctx->NextEvent};
    DWORD waitResult = WaitForMultipleObjects(3, waitArray, FALSE, INFINITE);

    if (waitResult == WAIT_OBJECT_0 + 0) {
      isActive = false;
      continue;
    }
    if (ctx->ReadIndex == ctx->WriteIndex) {
      Log->Info(L"Processed all commands, entering idle state.",
                GetCurrentThreadId(), __LONGFILE__);
      ctx->IsIdle = true;

      if (ctx->NotifyIdleState != nullptr) {
        ctx->NotifyIdleState(0);
      }

      continue;
    }

    cmd = ctx->Commands[ctx->ReadIndex];
    ctx->ReadIndex = (ctx->ReadIndex + 1) % ctx->MaxCommands;

    switch (cmd->Type) {
    case 1:
      Log->Info(L"Play SFX", GetCurrentThreadId(), __LONGFILE__);

      ctx->SFXCtx->SFXIndex = cmd->SFXIndex;
      ctx->SFXCtx->SleepDuration = 0.0;

      if (!SetEvent(ctx->SFXCtx->KickEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    case 2:
      Log->Info(L"Sleep", GetCurrentThreadId(), __LONGFILE__);

      ctx->SFXCtx->SFXIndex = -1;
      ctx->SFXCtx->SleepDuration = cmd->SleepDuration;

      if (!SetEvent(ctx->SFXCtx->KickEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    case 3:
      Log->Info(L"Play voice (plain text)", GetCurrentThreadId(), __LONGFILE__);

      ctx->VoiceCtx->IsSSML = false;
      ctx->VoiceCtx->Text = cmd->Text;

      if (!SetEvent(ctx->VoiceCtx->KickEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    case 4:
      Log->Info(L"Play voice (SSML)", GetCurrentThreadId(), __LONGFILE__);

      ctx->VoiceCtx->IsSSML = true;
      ctx->VoiceCtx->Text = cmd->Text;

      if (!SetEvent(ctx->VoiceCtx->KickEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    }
  }

  Log->Info(L"End command thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
