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

// Returns the URL where the doodle can be downloaded, e.g.
// https://www.google.com/async/newtab_mobile. This depends on the user's
// Google domain.
GURL GetGoogleDoodleURL(const GURL& google_base_url);

// Implements AppendQueryparamsToLogoURL, defined in logo_common.h, for both
// "Legacy" and "New" APIs. Using this relies on the caller having prepared the
// |logo_url| by calling AppendPreliminaryParamsToGoogleLogoURL() for example.
GURL GoogleAppendFingerprintParamToLogoURL(const GURL& logo_url,
                                           const std::string& fingerprint);

// Appends the correct queryparams ("Legacy" or "New"), based on the value of
// features::kUseDdljsonApi by delegating to GoogleNewAppendQueryparamsToLogoURL
// GoogleLegacyAppendQueryparamsToLogoURL.
GURL AppendPreliminaryParamsToGoogleLogoURL(bool gray_background,
                                            const GURL& logo_url);

// Returns the correct callbacks for parsing the response ("Legacy" or "New"),
// based on the value of features::kUseDdljsonApi.
ParseLogoResponse GetGoogleParseLogoResponseCallback(const GURL& base_url);

// Builds the persistent API URL for Google doodles (old newtab_mobile API).
GURL GoogleLegacyAppendQueryparamsToLogoURL(bool gray_background,
                                            const GURL& logo_url);

// Implements ParseLogoResponse, defined in logo_common.h, for Google doodles
// (old newtab_mobile API).
std::unique_ptr<EncodedLogo> GoogleLegacyParseLogoResponse(
    std::unique_ptr<std::string> response,
    base::Time response_time,
    bool* parsing_failed);

// Builds the persistent API URL for Google or third-party doodles
// (new ddljson API).
GURL GoogleNewAppendQueryparamsToLogoURL(bool gray_background,
                                         const GURL& logo_url);

// Implements ParseLogoResponse, defined in logo_common.h, for Google or
// third-party doodles (new ddljson API).
std::unique_ptr<EncodedLogo> GoogleNewParseLogoResponse(
    const GURL& base_url,
    std::unique_ptr<std::string> response,
    base::Time response_time,
    bool* parsing_failed);

}  // namespace search_provider_logos

#endif  // COMPONENTS_SEARCH_PROVIDER_LOGOS_GOOGLE_LOGO_API_H_
