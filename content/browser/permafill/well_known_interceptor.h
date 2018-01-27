// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PERMAFILL_WELL_KNOWN_INTERCEPTOR_H_
#define CONTENT_BROWSER_PERMAFILL_WELL_KNOWN_INTERCEPTOR_H_

#include "net/url_request/url_request_interceptor.h"

namespace permafill {

class WellKnownURLInterceptor : public net::URLRequestInterceptor {
 public:
  WellKnownURLInterceptor() {}
  ~WellKnownURLInterceptor() override {}
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WellKnownURLInterceptor);
};

}  // namespace permafill

#endif  // CONTENT_BROWSER_PERMAFILL_WELL_KNOWN_INTERCEPTOR_H_
