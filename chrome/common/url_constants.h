// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Contains constants for known URLs and portions thereof.
// Except for constants used in chrome/browser only. Those go in
// chrome/browser/url_constants.h.
// Except for WebUI UI/Host/SubPage constants. Those go in
// chrome/common/webui_url_constants.h.
//
// - Use the same order in this header and url_constants.cc.
// - Keep the constants sorted by name.
// - Put platform/feature specific constants towards the end in the appropriate
//   section.

#ifndef CHROME_COMMON_URL_CONSTANTS_H_
#define CHROME_COMMON_URL_CONSTANTS_H_

#include <stddef.h>

#include "build/build_config.h"
#include "chrome/common/features.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/common/url_constants.h"

namespace chrome {

// The chrome-native: scheme is used show pages rendered with platform specific
// widgets instead of using HTML.
extern const char kChromeNativeScheme[];

// Pages under chrome-search.
extern const char kChromeSearchLocalNtpHost[];
extern const char kChromeSearchLocalNtpUrl[];

// Host and URL for most visited iframes used on the Instant Extended NTP.
extern const char kChromeSearchMostVisitedHost[];
extern const char kChromeSearchMostVisitedUrl[];

// The chrome-search: scheme is served by the same backend as chrome:.  However,
// only specific URLDataSources are enabled to serve requests via the
// chrome-search: scheme.  See |InstantIOContext::ShouldServiceRequest| and its
// callers for details.  Note that WebUIBindings should never be granted to
// chrome-search: pages.  chrome-search: pages are displayable but not readable
// by external search providers (that are rendered by Instant renderer
// processes), and neither displayable nor readable by normal (non-Instant) web
// pages.  To summarize, a non-Instant process, when trying to access
// 'chrome-search://something', will bump up against the following:
//
//  1. Renderer: The display-isolated check in WebKit will deny the request,
//  2. Browser: Assuming they got by #1, the scheme checks in
//     URLDataSource::ShouldServiceRequest will deny the request,
//  3. Browser: for specific sub-classes of URLDataSource, like ThemeSource
//     there are additional Instant-PID checks that make sure the request is
//     coming from a blessed Instant process, and deny the request.
extern const char kChromeSearchScheme[];

// URL used to indicate that an extension resource load request was invalid.
extern const char kExtensionInvalidRequestURL[];

#if defined(OS_ANDROID)
extern const char kAndroidAppScheme[];
#endif

#if defined(OS_CHROMEOS)
extern const char kCrosScheme[];
#endif

}  // namespace chrome

#endif  // CHROME_COMMON_URL_CONSTANTS_H_
