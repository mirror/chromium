// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/origin_security_checker.h"

#include <string>
#include <vector>

#include "base/stl_util.h"
#import "ios/web/public/origin_util.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace web {

bool IsContextSecure(const GURL& url) {
  return url.is_valid() && web::IsOriginSecure(url) &&
         !base::ContainsValue(std::vector<std::string>{url::kDataScheme},
                              url.scheme());
}

}  // namespace web
