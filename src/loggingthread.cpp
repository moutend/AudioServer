#include <cpplogger/cpplogger.h>
#include <cpprest/http_client.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "context.h"
#include "loggingthread.h"
#include "util.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

extern Logger::Logger *Log;
extern wchar_t *LogServerAddr;

pplx::task<http_response> postRequest(json::value postData) {
  http_client client(LogServerAddr);

  return client.request(methods::POST, U(""), postData.serialize(),
                        U("application/json"));
}

DWORD WINAPI loggingThread(LPVOID context) {
  LoggingContext *ctx = static_cast<LoggingContext *>(context);

  if (ctx == nullptr) {
    return E_FAIL;
  }

  bool isActive{true};

  while (isActive) {
    HANDLE waitArray[1] = {ctx->QuitEvent};
    DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, 0);

    if (waitResult == WAIT_OBJECT_0 + 0) {
      isActive = false;
      continue;
    }

    Sleep(10000);

    if (Log->IsEmpty()) {
      continue;
    }

    json::value message = Log->ToJSON();

    try {
      postRequest(message).wait();
    } catch (...) {
      Log->Warn(L"Failed to send log messages", GetCurrentThreadId(),
                __LONGFILE__);

      continue;
    }

    Log->Clear();
  }

  return S_OK;
}
