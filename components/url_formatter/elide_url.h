// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for eliding URLs.

#ifndef COMPONENTS_URL_FORMATTER_ELIDE_URL_H_
#define COMPONENTS_URL_FORMATTER_ELIDE_URL_H_

#include <string>

#include "base/strings/string16.h"
#include "build/build_config.h"

class GURL;

namespace gfx {
class FontList;
}

namespace url {
class Origin;
struct Parsed;
}

namespace url_formatter {

// ElideUrl and Elide host require
// gfx::GetStringWidthF which is not implemented in Android
#if !defined(OS_ANDROID)
// This function takes a GURL object and elides it. It returns a string
// which composed of parts from subdomain, domain, path, filename and query.
// A "..." is added automatically at the end if the elided string is bigger
// than the |available_pixel_width|. For |available_pixel_width| == 0, a
// formatted, but un-elided, string is returned.
//
// Note: in RTL locales, if the URL returned by this function is going to be
// displayed in the UI, then it is likely that the string needs to be marked
// as an LTR string (using base::i18n::WrapStringWithLTRFormatting()) so that it
// is displayed properly in an RTL context. Please refer to
// http://crbug.com/6487 for more information.
base::string16 ElideUrl(const GURL& url,
                        const gfx::FontList& font_list,
                        float available_pixel_width);

// This method is similar to ElideUrl, but does not treat paths specially.
// First, everything following the host is elided, then the scheme is removed,
// and finally the host is elided from the left.
base::string16 SimpleElideUrl(const GURL& url,
                              const gfx::FontList& font_list,
                              float available_pixel_width,
                              url::Parsed* parsed);

// This function takes a GURL object and elides the host to fit within
// the given width. The function will never elide past the TLD+1 point,
// but after that, will leading-elide the domain name to fit the width.
// Example: http://sub.domain.com ---> "...domain.com", or "...b.domain.com"
// depending on the width.
base::string16 ElideHost(const GURL& host_url,
                         const gfx::FontList& font_list,
                         float available_pixel_width);
#endif  // !defined(OS_ANDROID)

// This function takes a formatted URL string, along with the original GURL and
// parsing results, and elides it to fit a specified width. Formatting is done
// separately for flexibility. Elision is performed according to the origin
// presentation guidance at:
// https://www.chromium.org/Home/chromium-security/enamel
// It will elide from the right until any path, query or reference components
// are gone, then elide from the left. By eliding from the left, the TLD+1 is
// preserved as long as possible, but will itself be chopped if necessary.
// The scheme is also chopped if helpful.
base::string16 SecurelyElideFormattedUrl(const GURL& url,
                                         const base::string16& url_string,
                                         const gfx::FontList& font_list,
                                         float available_pixel_width,
                                         bool preserve_filename,
                                         url::Parsed* parsed);

enum class SchemeDisplay {
  SHOW,
  OMIT_HTTP_AND_HTTPS,
  // Omit cryptographic (i.e. https and wss).
  OMIT_CRYPTOGRAPHIC,
};

// This is a convenience function for formatting a URL in a concise and
// human-friendly way, to help users make security-related decisions (or in
// other circumstances when people need to distinguish sites, origins, or
// otherwise-simplified URLs from each other).
//
// Internationalized domain names (IDN) will be presented in Unicode if
// they're regarded safe except that domain names with RTL characters
// will still be in ACE/punycode for now (http://crbug.com/650760).
// See http://dev.chromium.org/developers/design-documents/idn-in-google-chrome
// for details on the algorithm.
//
// - Omits the path for standard schemes, excepting file and filesystem.
// - Omits the port if it is the default for the scheme.
//
// Do not use this for URLs which will be parsed or sent to other applications.
//
// Generally, prefer SchemeDisplay::SHOW to omitting the scheme unless there is
// plenty of indication as to whether the origin is secure elsewhere in the UX.
// For example, in Chrome's Page Info Bubble, there are icons and strings
// indicating origin (non-)security. But in the HTTP Basic Auth prompt (for
// example), the scheme may be the only indicator.
base::string16 FormatUrlForSecurityDisplay(
    const GURL& origin,
    const SchemeDisplay scheme_display = SchemeDisplay::SHOW);

// This is a convenience function for formatting a url::Origin in a concise and
// human-friendly way, to help users make security-related decisions.
//
// - Omits the port if it is 0 or the default for the scheme.
//
// Do not use this for origins which will be parsed or sent to other
// applications.
//
// Generally, prefer SchemeDisplay::SHOW to omitting the scheme unless there is
// plenty of indication as to whether the origin is secure elsewhere in the UX.
base::string16 FormatOriginForSecurityDisplay(
    const url::Origin& origin,
    const SchemeDisplay scheme_display = SchemeDisplay::SHOW);

}  // namespace url_formatter

#endif  // COMPONENTS_URL_FORMATTER_ELIDE_URL_H_
