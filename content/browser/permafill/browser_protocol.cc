// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/permafill/browser_protocol.h"

#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_redirect_job.h"

namespace permafill {

class BrowserProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  BrowserProtocolHandler() {}

  ~BrowserProtocolHandler() override {}

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserProtocolHandler);
};

// Creates URLRequestJobs for browser:// URLs.
net::URLRequestJob* BrowserProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  // browser://permafill-name?fallback-url

  std::string permafill_name = request->url().host();

  if (permafill_name != "test") {
    std::string permafill_fallback = request->url().query();
    GURL permafill_fallback_url(permafill_fallback);
    if (!permafill_fallback_url.is_valid()) {
      return new net::URLRequestErrorJob(request, network_delegate,
                                         net::ERR_FAILED);
    }
    return new net::URLRequestRedirectJob(
        request, network_delegate, permafill_fallback_url,
        net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
        "Fallback");
  }

  return new net::URLRequestRedirectJob(
      request, network_delegate,
      GURL("chrome-extension://acihfchgdiicfpkconamoakocligclao/test/index.js"),
      net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT, "Permafill");
}

std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
CreateBrowserProtocolHandler() {
  return std::make_unique<BrowserProtocolHandler>();
}

}  // namespace permafill
