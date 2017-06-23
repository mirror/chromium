// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_PROVIDER_LOGOS_GOOGLE_LOGO_API_H_
#define COMPONENTS_SEARCH_PROVIDER_LOGOS_GOOGLE_LOGO_API_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "components/search_provider_logos/logo_common.h"
#include "url/gurl.h"

namespace search_provider_logos {

// Implements AppendQueryparamsToLogoURL, defined in logo_tracker.h, for Google
// doodles (old newtab_mobile API).
GURL GoogleLegacyAppendQueryparamsToLogoURL(bool gray_background,
                                            const GURL& logo_url,
                                            const std::string& fingerprint);

// Implements ParseLogoResponse, defined in logo_tracker.h, for Google doodles
// (old newtab_mobile API).
std::unique_ptr<EncodedLogo> GoogleLegacyParseLogoResponse(
    std::unique_ptr<std::string> response,
    base::Time response_time,
    bool* parsing_failed);

// Implements AppendQueryparamsToLogoURL, defined in logo_tracker.h, for Google
// doodles (new ddljson API).
GURL GoogleNewAppendQueryparamsToLogoURL(bool gray_background,
                                         const GURL& logo_url,
                                         const std::string& fingerprint);

// Implements ParseLogoResponse, defined in logo_tracker.h, for Google doodles
// (new ddljson API).
std::unique_ptr<EncodedLogo> GoogleNewParseLogoResponse(
    const GURL& base_url,
    std::unique_ptr<std::string> response,
    base::Time response_time,
    bool* parsing_failed);

}  // namespace search_provider_logos

#endif  // COMPONENTS_SEARCH_PROVIDER_LOGOS_GOOGLE_LOGO_API_H_
