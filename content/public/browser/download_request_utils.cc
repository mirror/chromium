// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/download_request_utils.h"

#include "content/browser/download/download_request_core.h"

namespace content {

// static
std::string DownloadRequestUtils::GetRequestOriginFromRequest(
    const net::URLRequest* request) {
  return DownloadRequestCore::GetRequestOriginFromRequest(request);
}

}  // namespace content