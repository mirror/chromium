// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_ONE_GOOGLE_BAR_ONE_GOOGLE_BAR_AUTH_UTIL_H_
#define CHROME_BROWSER_SEARCH_ONE_GOOGLE_BAR_ONE_GOOGLE_BAR_AUTH_UTIL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"

class GURL;

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace auth_util {

void GetAuthToken(net::URLRequestContextGetter* request_context_getter,
                  const GURL& google_base_url,
                  base::OnceCallback<void(const std::string&)> callback);

}  // namespace auth_util

#endif  // CHROME_BROWSER_SEARCH_ONE_GOOGLE_BAR_ONE_GOOGLE_BAR_AUTH_UTIL_H_
