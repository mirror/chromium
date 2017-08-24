// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/background_fetch_response.h"

namespace content {

BackgroundFetchResponse::BackgroundFetchResponse(
    const std::vector<GURL>& url_chain,
    const scoped_refptr<const net::HttpResponseHeaders>& headers)
    : url_chain(url_chain), headers(headers) {}

BackgroundFetchResponse::~BackgroundFetchResponse() {}

}  // namespace content
