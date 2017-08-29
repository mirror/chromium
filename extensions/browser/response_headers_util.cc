// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/response_headers_util.h"

#include "base/strings/string_util.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "google_apis/gaia/gaia_urls.h"
#include "url/gurl.h"

namespace extensions {

bool ShouldHiderResponseHeader(const GURL& url,
                               const std::string& header_name) {
  return ((url.host() == GaiaUrls::GetInstance()->gaia_url().host()) &&
          (base::CompareCaseInsensitiveASCII(
               header_name, signin::kDiceResponseHeader) == 0));
}

}  // namespace extensions
