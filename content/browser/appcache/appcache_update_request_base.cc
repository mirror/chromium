// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_update_request_base.h"
#include "content/browser/appcache/appcache_update_url_request.h"
#include "net/url_request/url_request_context.h"

namespace content {

AppCacheUpdateRequestBase::~AppCacheUpdateRequestBase() {}

// static
std::unique_ptr<AppCacheUpdateRequestBase> AppCacheUpdateRequestBase::Create(
    net::URLRequestContext* request_context,
    const GURL& url,
    URLFetcher* fetcher) {
  return std::unique_ptr<AppCacheUpdateRequestBase>(
      new AppCacheUpdateURLRequest(request_context, url, fetcher));
}

AppCacheUpdateRequestBase::AppCacheUpdateRequestBase() {}

}  // namespace content
