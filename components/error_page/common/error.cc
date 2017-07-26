// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/error_page/common/error.h"

namespace error_page {

constexpr char Error::kHttpErrorDomain[];
constexpr char Error::kDnsProbeErrorDomain[];

Error::Error(const GURL& url,
             const std::string& domain,
             int reason,
             bool stale_copy_in_cache)
    : url_(url),
      domain_(domain),
      reason_(reason),
      stale_copy_in_cache_(stale_copy_in_cache) {}

Error Error::NetError(const GURL& url, int reason, bool stale_copy_in_cache) {
  return Error(url, "net", reason, stale_copy_in_cache);
}

Error Error::HttpError(const GURL& url, int http_status_code) {
  return Error(url, "http", http_status_code, false);
}

Error Error::DnsProbeError(const GURL& url,
                           int status,
                           bool stale_copy_in_cache) {
  return Error(url, "dns", status, false);
}

}  // namespace error_page
