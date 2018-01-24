// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PERMAFILL_BROWSER_PROTOCOL_H_
#define CONTENT_BROWSER_PERMAFILL_BROWSER_PROTOCOL_H_

#include <memory>

#include "net/url_request/url_request_job_factory.h"

namespace permafill {

// Creates the handlers for the browser:// scheme.
std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
CreateBrowserProtocolHandler();

}  // namespace permafill

#endif  // CONTENT_BROWSER_PERMAFILL_BROWSER_PROTOCOL_H_
