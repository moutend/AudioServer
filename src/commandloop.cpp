#include <cpplogger/cpplogger.h>
#include <windows.h>

#include "commandloop.h"
#include "context.h"
#include "util.h"

#include <strsafe.h>

extern Logger::Logger *Log;

DWORD WINAPI commandLoop(LPVOID context) {
  Log->Info(L"Start command loop thread", GetCurrentThreadId(), __LONGFILE__);

  CommandLoopContext *ctx = static_cast<CommandLoopContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain ctx", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Command *cmd{nullptr};
  bool isActive{true};

  while (isActive) {
    HANDLE waitArray[4] = {ctx->QuitEvent, ctx->PushEvent,
                           ctx->VoiceLoopCtx->NextEvent,
                           ctx->SFXLoopCtx->NextEvent};
    DWORD waitResult = WaitForMultipleObjects(4, waitArray, FALSE, INFINITE);

    if (waitResult == WAIT_OBJECT_0 + 0) {
      isActive = false;
      continue;
    }
    if (ctx->ReadIndex == ctx->WriteIndex) {
      ctx->IsIdle = true;
      continue;
    }
    switch (waitResult) {
    case WAIT_OBJECT_0 + 1: // ctx->PushEvent
      ctx->VoiceLoopCtx->VoiceEngine->FadeOut();
      ctx->SFXLoopCtx->SFXEngine->FadeOut();
      break;
    }

    cmd = ctx->Commands[ctx->ReadIndex];
    ctx->ReadIndex = (ctx->ReadIndex + 1) % ctx->MaxCommands;

    switch (cmd->Type) {
    case 1:
      Log->Info(L"Play SFX", GetCurrentThreadId(), __LONGFILE__);

      ctx->SFXLoopCtx->SFXIndex = cmd->SFXIndex;

      if (!SetEvent(ctx->SFXLoopCtx->FeedEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    case 2:
      Log->Info(L"Wait", GetCurrentThreadId(), __LONGFILE__);

      ctx->SFXLoopCtx->SFXIndex = -1;
      ctx->SFXLoopCtx->WaitDuration = cmd->WaitDuration;

      if (!SetEvent(ctx->SFXLoopCtx->FeedEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    case 3:
      Log->Info(L"Play voice generated from plain text", GetCurrentThreadId(),
                __LONGFILE__);

      ctx->VoiceLoopCtx->IsSSML = false;
      ctx->VoiceLoopCtx->Text = cmd->Text;

      if (!SetEvent(ctx->VoiceLoopCtx->FeedEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    case 4:
      Log->Info(L"Play voice generated from SSML", GetCurrentThreadId(),
                __LONGFILE__);

      ctx->VoiceLoopCtx->IsSSML = true;
      ctx->VoiceLoopCtx->Text = cmd->Text;

      if (!SetEvent(ctx->VoiceLoopCtx->FeedEvent)) {
        Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
      }

      break;
    }
  }

  Log->Info(L"End command loop thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}
