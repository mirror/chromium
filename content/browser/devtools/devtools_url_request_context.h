// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_CONTEXT_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_CONTEXT_H_

#include "net/url_request/url_request_context.h"
#include "net/http/http_transaction_factory.h"
#include "content/browser/devtools/devtools_http_transaction_factory.h"

namespace content {

class DevtoolsURLRequestContext : public net::URLRequestContext {
public:
  DevtoolsURLRequestContext(const net::URLRequestContext* base_url_context,
    scoped_refptr<DevToolsURLRequestInterceptor::State> state)
    : devtools_http_transaction_factory_(state, base_url_context->http_transaction_factory()) {
      CopyFrom(base_url_context);
      set_http_transaction_factory(&devtools_http_transaction_factory_);
    }

private:
  mutable DevtoolsHttpTransactionFactory devtools_http_transaction_factory_;

};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_CONTEXT_H_
