// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ERROR_PAGE_COMMON_ERROR_H_
#define COMPONENTS_ERROR_PAGE_COMMON_ERROR_H_

#include <string>
#include "url/gurl.h"

namespace error_page {

class Error {
 public:
  static Error NetError(const GURL& url, int reason, bool stale_copy_in_cache);
  static Error HttpError(const GURL& url, int status);
  static Error DnsProbeError(const GURL& url,
                             int status,
                             bool stale_copy_in_cache);

  const GURL& url() const { return url_; }
  const std::string& domain() const { return domain_; }
  int reason() const { return reason_; }
  bool stale_copy_in_cache() const { return stale_copy_in_cache_; }

  // For dns probe errors.
  static constexpr char kDnsProbeErrorDomain[] = "dnsprobe";
  // For http errors.
  static constexpr char kHttpErrorDomain[] = "http";
  // Use net::kErroDomain for network errors.

 private:
  Error(const GURL& url,
        const std::string& domain,
        int reason,
        bool stale_copy_in_cache);

  GURL url_;
  std::string domain_;
  int reason_;
  bool stale_copy_in_cache_ = false;
};

}  // namespace error_page

#endif  // COMPONENTS_ERROR_PAGE_COMMON_ERROR_H_
