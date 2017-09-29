// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/slow_download_http_response.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"

namespace content {

namespace {

static bool g_should_finish_download = false;

void SendResponseBodyDone(const net::test_server::SendBytesCallback& send,
                          const net::test_server::SendCompleteCallback& done,
                          int next_data_size);

// Sends the response body with the given size.
void SendResponseBody(const net::test_server::SendBytesCallback& send,
                      const net::test_server::SendCompleteCallback& done,
                      int data_size) {
  if (g_should_finish_download) {
    done.Run();
    return;
  }

  std::string response(data_size, '*');
  int next_data_size = data_size == SlowDownloadHttpResponse::kFirstDownloadSize
                           ? SlowDownloadHttpResponse::kSecondDownloadSize
                           : 0;
  send.Run(response,
           base::Bind(&SendResponseBodyDone, send, done, next_data_size));
}

// Called when the response body was sucessfully sent.
void SendResponseBodyDone(const net::test_server::SendBytesCallback& send,
                          const net::test_server::SendCompleteCallback& done,
                          int next_data_size) {
  if (g_should_finish_download) {
    done.Run();
    return;
  }

  if (next_data_size > 0) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&SendResponseBody, send, done, next_data_size),
        base::TimeDelta::FromMilliseconds(100));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&SendResponseBodyDone, send, done, 0),
        base::TimeDelta::FromMilliseconds(100));
  }
}

}  // namespace

// static
const char SlowDownloadHttpResponse::kSlowDownloadHostName[] =
    "url.handled.by.slow.download";
const char SlowDownloadHttpResponse::kUnknownSizeUrl[] =
    "/download-unknown-size";
const char SlowDownloadHttpResponse::kKnownSizeUrl[] = "/download-known-size";
const char SlowDownloadHttpResponse::kFinishDownloadUrl[] = "/download-finish";

const int SlowDownloadHttpResponse::kFirstDownloadSize = 1024 * 35;
const int SlowDownloadHttpResponse::kSecondDownloadSize = 1024 * 10;

// static
std::unique_ptr<net::test_server::HttpResponse>
SlowDownloadHttpResponse::HandleSlowDownloadRequest(
    const net::test_server::HttpRequest& request) {
  if (request.relative_url != kUnknownSizeUrl &&
      request.relative_url != kKnownSizeUrl &&
      request.relative_url != kFinishDownloadUrl) {
    return nullptr;
  }
  auto response =
      base::MakeUnique<SlowDownloadHttpResponse>(request.relative_url);
  return std::move(response);
}

SlowDownloadHttpResponse::SlowDownloadHttpResponse(const std::string& url)
    : url_(url) {}

SlowDownloadHttpResponse::~SlowDownloadHttpResponse() = default;

void SlowDownloadHttpResponse::SendResponse(
    const net::test_server::SendBytesCallback& send,
    const net::test_server::SendCompleteCallback& done) {
  std::string response;
  response.append("HTTP/1.1 200 OK\r\n");
  if (base::LowerCaseEqualsASCII(kFinishDownloadUrl, url_)) {
    response.append("Content-type: text/plain\r\n");
    response.append("\r\n");

    g_should_finish_download = true;
    send.Run(response, done);
  } else {
    response.append("Content-type: application/octet-stream\r\n");
    response.append("Cache-Control: max-age=0\r\n");

    if (base::LowerCaseEqualsASCII(kKnownSizeUrl, url_)) {
      response.append(base::StringPrintf(
          "Content-Length: %d\r\n", kFirstDownloadSize + kSecondDownloadSize));
    }
    response.append("\r\n");
    send.Run(response,
             base::Bind(&SendResponseBody, send, done, kFirstDownloadSize));
  }
}

}  // namespace content
