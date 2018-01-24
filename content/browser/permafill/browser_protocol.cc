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

bool IsImplemented(const std::string& permafill) {
  // TODO(hiroshige): Supported permafills are hard-coded here.
  if (permafill == "test")
    return true;
  return false;
}

// Creates URLRequestJobs for browser:// URLs.
//
// https://docs.google.com/document/d/1VbU4z7xtU_kzuLAcj38KKL5qoOr2UYNUJW8vZB2AcWc/edit#heading=h.d8xhrnoec1xa
net::URLRequestJob* BrowserProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  // 1.a. If request's method is not `GET`, then return a network error.
  if (request->method() != "GET") {
    return new net::URLRequestErrorJob(request, network_delegate,
                                       net::ERR_FAILED);
  }

  // 1.c. Let path be request's current url's path[0].
  // 1.d. Let permafill be the portion of path before the first U+007C (|),
  // or all of path if no U+007C (|) is present.
  // 1.e. Let fallback be the portion of path after the first U+007C (|),
  // or null if no U+007C (|) is present.
  //
  // TODO(hiroshige): Currently browser://<permafill-name>?<fallback-url>
  // is used instead of browser:<permafill-name>|<fallback-url>.
  const std::string permafill = request->url().host();
  const std::string fallback = request->url().query();

  // 1.f. If the browser implements the permafill identified by permafill, then:
  if (IsImplemented(permafill)) {
    // TODO(hiroshige): Currently implemented as a redirect to chrome-extension.
    return new net::URLRequestRedirectJob(
        request, network_delegate,
        GURL("chrome-extension://acihfchgdiicfpkconamoakocligclao/" +
             permafill + "/index.js"),
        net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
        "Permafill");
  }

  // 1.g. Otherwise, if fallback is non-null, then:
  if (fallback != "") {
    // 1.g.1. Let fallbackURL be the result of parsing fallback. If this fails,
    // then return a network error.
    GURL fallback_url(fallback);
    if (!fallback_url.is_valid()) {
      return new net::URLRequestErrorJob(request, network_delegate,
                                         net::ERR_FAILED);
    }

    // 1.g.2. Let response be a new response.
    // 1.g.3. Set response's status to 307.
    // 1.g.4. Let fallbackURLBytes be fallbackURL, serialized and UTF-8 encoded.
    // 1.g.5. Append `Location`/fallbackURL to response's header list.
    // 1.g.6. Return response.
    return new net::URLRequestRedirectJob(
        request, network_delegate, fallback_url,
        net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
        "Fallback");
  }

  // 1.h. Otherwise, the browser does not implement the given permafill and no
  // fallback was provided; return a network error.
  return new net::URLRequestErrorJob(request, network_delegate,
                                     net::ERR_FAILED);
}

std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
CreateBrowserProtocolHandler() {
  return std::make_unique<BrowserProtocolHandler>();
}

}  // namespace permafill
