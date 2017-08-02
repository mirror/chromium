// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/url_request/url_request_hanging_get_job.h"

#include <string>

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_filter.h"

namespace net {
namespace {

const char kMockHostname[] = "mock.hanging.get";

GURL GetMockUrl(const std::string& scheme, const std::string& hostname) {
  return GURL(scheme + "://" + hostname + "/");
}

class MockJobInterceptor : public URLRequestInterceptor {
 public:
  MockJobInterceptor() {}
  ~MockJobInterceptor() override {}

  // URLRequestInterceptor implementation
  URLRequestJob* MaybeInterceptRequest(
      URLRequest* request,
      NetworkDelegate* network_delegate) const override {
    return new URLRequestHangingGetJob(request, network_delegate);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockJobInterceptor);
};

}  // namespace

URLRequestHangingGetJob::URLRequestHangingGetJob(
    URLRequest* request,
    NetworkDelegate* network_delegate)
    : URLRequestJob(request, network_delegate), weak_factory_(this) {}

void URLRequestHangingGetJob::Start() {
  // Do nothing.
}

URLRequestHangingGetJob::~URLRequestHangingGetJob() {}

// static
void URLRequestHangingGetJob::AddUrlHandler() {
  // Add |hostname| to URLRequestFilter for HTTP and HTTPS.
  URLRequestFilter* filter = URLRequestFilter::GetInstance();
  filter->AddHostnameInterceptor("http", kMockHostname,
                                 base::MakeUnique<MockJobInterceptor>());
  filter->AddHostnameInterceptor("https", kMockHostname,
                                 base::MakeUnique<MockJobInterceptor>());
}

// static
GURL URLRequestHangingGetJob::GetMockHttpUrl() {
  return GetMockUrl("http", kMockHostname);
}

// static
GURL URLRequestHangingGetJob::GetMockHttpsUrl() {
  return GetMockUrl("https", kMockHostname);
}

}  // namespace net
