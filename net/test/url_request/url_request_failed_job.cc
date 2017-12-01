// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/url_request/url_request_failed_job.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_interceptor.h"

namespace net {

namespace {

const char kMockHostname[] = "mock.failed.request";

const char kResponseParameterName[] = "response";

// String names of failure phases matching FailurePhase enum.
const char* kFailurePhase[]{
    "start",      // START
    "readsync",   // READ_SYNC
    "readasync",  // READ_ASYNC
};

static_assert(arraysize(kFailurePhase) ==
                  URLRequestFailedJob::FailurePhase::MAX_FAILURE_PHASE,
              "kFailurePhase must match FailurePhase enum");

URLRequestFailedJob::ReadRawDataCompleteInterceptor
    g_read_raw_data_complete_interceptor;

class MockJobInterceptor : public URLRequestInterceptor {
 public:
  MockJobInterceptor() = default;
  ~MockJobInterceptor() override = default;

  // URLRequestJobFactory::ProtocolHandler implementation:
  URLRequestJob* MaybeInterceptRequest(
      URLRequest* request,
      NetworkDelegate* network_delegate) const override {
    int net_error = OK;
    URLRequestFailedJob::FailurePhase phase =
        URLRequestFailedJob::FailurePhase::MAX_FAILURE_PHASE;
    for (size_t i = 0; i < arraysize(kFailurePhase); i++) {
      std::string phase_error_string;
      if (GetValueForKeyInQuery(request->url(), kFailurePhase[i],
                                &phase_error_string)) {
        if (base::StringToInt(phase_error_string, &net_error)) {
          phase = static_cast<URLRequestFailedJob::FailurePhase>(i);
          break;
        }
      }
    }
    std::string response;
    if (GetValueForKeyInQuery(request->url(), kResponseParameterName,
                              &response)) {
      DCHECK_EQ(phase, URLRequestFailedJob::READ_ASYNC);
    }
    return new URLRequestFailedJob(request, network_delegate, phase, net_error,
                                   response);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockJobInterceptor);
};

GURL GetMockUrl(const std::string& scheme,
                const std::string& hostname,
                URLRequestFailedJob::FailurePhase phase,
                int net_error,
                const std::string& response = std::string()) {
  CHECK_GE(phase, URLRequestFailedJob::FailurePhase::START);
  CHECK_LE(phase, URLRequestFailedJob::FailurePhase::READ_ASYNC);
  CHECK_LT(net_error, OK);
  GURL url(scheme + "://" + hostname + "/error?" + kFailurePhase[phase] + "=" +
           base::IntToString(net_error));
  if (!response.empty())
    url = net::AppendQueryParameter(url, kResponseParameterName, response);
  return url;
}

}  // namespace

URLRequestFailedJob::URLRequestFailedJob(URLRequest* request,
                                         NetworkDelegate* network_delegate,
                                         FailurePhase phase,
                                         int net_error,
                                         std::string response)
    : URLRequestJob(request, network_delegate),
      phase_(phase),
      net_error_(net_error),
      total_received_bytes_(0),
      response_(std::move(response)),
      weak_factory_(this) {
  CHECK_GE(phase, URLRequestFailedJob::FailurePhase::START);
  CHECK_LE(phase, URLRequestFailedJob::FailurePhase::READ_ASYNC);
  CHECK_LT(net_error, OK);
}

URLRequestFailedJob::URLRequestFailedJob(URLRequest* request,
                                         NetworkDelegate* network_delegate,
                                         int net_error)
    : URLRequestFailedJob(request,
                          network_delegate,
                          START,
                          net_error,
                          std::string()) {}

void URLRequestFailedJob::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&URLRequestFailedJob::StartAsync, weak_factory_.GetWeakPtr()));
}

int URLRequestFailedJob::ReadRawData(IOBuffer* buf, int buf_size) {
  CHECK(phase_ == READ_SYNC || phase_ == READ_ASYNC);
  if (net_error_ == ERR_IO_PENDING || phase_ == READ_SYNC)
    return net_error_;

  int result = net_error_;

  if (!response_.empty()) {
    size_t chars_written =
        base::StringPiece(response_).copy(buf->data(), buf_size, 0);
    total_received_bytes_ += chars_written;
    response_ = response_.substr(chars_written);
    result = chars_written;
  }

  auto task = base::BindOnce(&URLRequestFailedJob::ReadRawDataComplete,
                             weak_factory_.GetWeakPtr(), result);
  if (g_read_raw_data_complete_interceptor)
    g_read_raw_data_complete_interceptor.Run(std::move(task), result);
  else
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(task));

  return ERR_IO_PENDING;
}

void URLRequestFailedJob::GetResponseInfo(HttpResponseInfo* info) {
  *info = response_info_;
}

void URLRequestFailedJob::PopulateNetErrorDetails(
    NetErrorDetails* details) const {
  if (net_error_ == ERR_QUIC_PROTOCOL_ERROR) {
    details->quic_connection_error = QUIC_INTERNAL_ERROR;
  }
}

int64_t URLRequestFailedJob::GetTotalReceivedBytes() const {
  return total_received_bytes_;
}

// static
void URLRequestFailedJob::AddUrlHandler() {
  return AddUrlHandlerForHostname(kMockHostname);
}

// static
void URLRequestFailedJob::AddUrlHandlerForHostname(
    const std::string& hostname) {
  URLRequestFilter* filter = URLRequestFilter::GetInstance();
  // Add |hostname| to URLRequestFilter for HTTP and HTTPS.
  filter->AddHostnameInterceptor(
      "http", hostname,
      std::unique_ptr<URLRequestInterceptor>(new MockJobInterceptor()));
  filter->AddHostnameInterceptor(
      "https", hostname,
      std::unique_ptr<URLRequestInterceptor>(new MockJobInterceptor()));
}

// static
GURL URLRequestFailedJob::GetMockHttpUrl(int net_error) {
  return GetMockHttpUrlForHostname(net_error, kMockHostname);
}

// static
GURL URLRequestFailedJob::GetMockHttpsUrl(int net_error) {
  return GetMockHttpsUrlForHostname(net_error, kMockHostname);
}

// static
GURL URLRequestFailedJob::GetMockHttpUrlWithFailurePhase(FailurePhase phase,
                                                         int net_error) {
  return GetMockUrl("http", kMockHostname, phase, net_error);
}

// static
GURL URLRequestFailedJob::GetMockHttpUrlWithPartialResponse(
    const std::string& response,
    int net_error) {
  return GetMockUrl("http", kMockHostname, READ_ASYNC, net_error, response);
}

// static
GURL URLRequestFailedJob::GetMockHttpUrlForHostname(
    int net_error,
    const std::string& hostname) {
  return GetMockUrl("http", hostname, START, net_error);
}

// static
GURL URLRequestFailedJob::GetMockHttpsUrlForHostname(
    int net_error,
    const std::string& hostname) {
  return GetMockUrl("https", hostname, START, net_error);
}

// static
void URLRequestFailedJob::SetReadRawDataCompleteInterceptor(
    ReadRawDataCompleteInterceptor interceptor) {
  g_read_raw_data_complete_interceptor = std::move(interceptor);
}

URLRequestFailedJob::~URLRequestFailedJob() = default;

void URLRequestFailedJob::StartAsync() {
  if (phase_ == START) {
    if (net_error_ != ERR_IO_PENDING) {
      NotifyStartError(URLRequestStatus(URLRequestStatus::FAILED, net_error_));
      return;
    }
    return;
  }
  const std::string headers = "HTTP/1.1 200 OK";
  response_info_.headers = new net::HttpResponseHeaders(headers);
  total_received_bytes_ = headers.size();
  NotifyHeadersComplete();
}

}  // namespace net
