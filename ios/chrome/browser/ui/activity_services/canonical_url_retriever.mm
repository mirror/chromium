// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/canonical_url_retriever.h"

#include "base/mac/bind_objc_block.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#import "ios/chrome/browser/procedural_block_types.h"
#import "ios/web/public/web_state/web_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Script to access the canonical URL from a web page.
const char kCanonicalURLScript[] =
    "(function() {"
    "  var linkNode = document.querySelector(\"link[rel='canonical']\");"
    "  return linkNode ? linkNode.getAttribute(\"href\") : \"\";"
    "})()";

// The histogram key to report the result of the Canonical URL retrieval.
const char kCanonicalURLResultHistogram[] = "IOS.CanonicalURLResult";

// Used to report the result of the retrieval of the Canonical URL.
enum CanonicalURLResult {
  // The canonical URL retrieval failed because the visible URL is not HTTPS.
  FAILED_VISIBLE_URL_NOT_HTTPS = 0,

  // The canonical URL retrieval failed because the canonical URL is not HTTPS
  // (but the visible URL is).
  FAILED_CANONICAL_URL_NOT_HTTPS,

  // The canonical URL retrieval failed because the page did not define one.
  FAILED_NO_CANONICAL_URL_DEFINED,

  // The canonical URL retrieval failed because the page's canonical URL was
  // invalid.
  FAILED_CANONICAL_URL_INVALID,

  // The canonical URL retrieval succeeded. The retrieved canonical URL is
  // different from the visible URL.
  SUCCESS_CANONICAL_URL_DIFFERENT_FROM_VISIBLE,

  // The canonical URL retrieval succeeded. The retrieved canonical URL is the
  // same as the visible URL.
  SUCCESS_CANONICAL_URL_SAME_AS_VISIBLE,

  // The count of canonical URL results.
  CANONICAL_URL_RESULT_COUNT
};

// Logs the result if a valid canonical URL is found. |canonical_url| is the
// canonical URL that was found, and |visible_url| is the page's URL that is
// visible in the omnibox.
void LogValidCanonicalUrlFoundResult(const GURL& canonical_url,
                                     const GURL& visible_url) {
  // Check if |canonical_url| is HTTPS to determine result.
  if (!canonical_url.SchemeIsCryptographic()) {
    UMA_HISTOGRAM_ENUMERATION(kCanonicalURLResultHistogram,
                              FAILED_CANONICAL_URL_NOT_HTTPS,
                              CANONICAL_URL_RESULT_COUNT);
  } else {
    // Check whether |visible_url| and |canonical_url| are the same to determine
    // which success bucket to log.
    if (visible_url == canonical_url) {
      UMA_HISTOGRAM_ENUMERATION(kCanonicalURLResultHistogram,
                                SUCCESS_CANONICAL_URL_SAME_AS_VISIBLE,
                                CANONICAL_URL_RESULT_COUNT);
    } else {
      UMA_HISTOGRAM_ENUMERATION(kCanonicalURLResultHistogram,
                                SUCCESS_CANONICAL_URL_DIFFERENT_FROM_VISIBLE,
                                CANONICAL_URL_RESULT_COUNT);
    }
  }
}

// Converts a |value| to a GURL. Returns an empty GURL if |value| is not a valid
// URL.
GURL UrlFromValue(const base::Value* value) {
  GURL canonical_url;

  if (value && value->is_string()) {
    canonical_url = GURL(value->GetString());
  }

  // Log result if no canonical URL is found.
  if (canonical_url.is_empty()) {
    UMA_HISTOGRAM_ENUMERATION(kCanonicalURLResultHistogram,
                              FAILED_NO_CANONICAL_URL_DEFINED,
                              CANONICAL_URL_RESULT_COUNT);
  } else if (!canonical_url.is_valid()) {
    // Log result if an invalid canonical URL is found.
    UMA_HISTOGRAM_ENUMERATION(kCanonicalURLResultHistogram,
                              FAILED_CANONICAL_URL_INVALID,
                              CANONICAL_URL_RESULT_COUNT);
  }

  return canonical_url.is_valid() ? canonical_url : GURL::EmptyGURL();
}

}  // namespace

namespace activity_services {
void RetrieveCanonicalUrl(web::WebState* web_state,
                          ProceduralBlockWithURL completion) {
  // Do not use the canonical URL if the page is not secured with HTTPS.
  const GURL& visible_url = web_state->GetVisibleURL();
  if (!visible_url.SchemeIsCryptographic()) {
    UMA_HISTOGRAM_ENUMERATION(kCanonicalURLResultHistogram,
                              FAILED_VISIBLE_URL_NOT_HTTPS,
                              CANONICAL_URL_RESULT_COUNT);
    completion(GURL::EmptyGURL());
    return;
  }

  void (^javascript_completion)(const base::Value*) =
      ^(const base::Value* value) {
        GURL canonical_url = UrlFromValue(value);

        LogValidCanonicalUrlFoundResult(canonical_url, visible_url);

        // If the canonical URL is not HTTPS, pass an empty GURL so that there
        // is not a downgrade from the HTTPS visible URL.
        completion(canonical_url.SchemeIsCryptographic() ? canonical_url
                                                         : GURL::EmptyGURL());
      };

  web_state->ExecuteJavaScript(base::UTF8ToUTF16(kCanonicalURLScript),
                               base::BindBlockArc(javascript_completion));
}
}  // namespace activity_services
