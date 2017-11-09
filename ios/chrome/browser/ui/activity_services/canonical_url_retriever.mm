// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/canonical_url_retriever.h"
#import "ios/chrome/browser/ui/activity_services/canonical_url_retriever_private.h"

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

// Logs the result if a valid canonical URL is found. |canonical_url| is the
// canonical URL that was found, and |visible_url| is the page's URL that is
// visible in the omnibox.
void LogValidCanonicalUrlFoundResult(const GURL& canonical_url,
                                     const GURL& visible_url) {
  // Check if |canonical_url| is HTTPS to determine result.
  if (!canonical_url.SchemeIsCryptographic()) {
    UMA_HISTOGRAM_ENUMERATION(activity_services::kCanonicalURLResultHistogram,
                              activity_services::FAILED_CANONICAL_URL_NOT_HTTPS,
                              activity_services::CANONICAL_URL_RESULT_COUNT);
  } else {
    // Check whether |visible_url| and |canonical_url| are the same to determine
    // which success bucket to log.
    if (visible_url == canonical_url) {
      UMA_HISTOGRAM_ENUMERATION(
          activity_services::kCanonicalURLResultHistogram,
          activity_services::SUCCESS_CANONICAL_URL_SAME_AS_VISIBLE,
          activity_services::CANONICAL_URL_RESULT_COUNT);
    } else {
      UMA_HISTOGRAM_ENUMERATION(
          activity_services::kCanonicalURLResultHistogram,
          activity_services::SUCCESS_CANONICAL_URL_DIFFERENT_FROM_VISIBLE,
          activity_services::CANONICAL_URL_RESULT_COUNT);
    }
  }
}

// Converts a |value| to a GURL. Returns an empty GURL if |value| is not a valid
// URL.
GURL UrlFromValue(const base::Value* value) {
  GURL canonical_url;
  bool canonical_url_found = false;

  if (value && value->is_string() && !value->GetString().empty()) {
    canonical_url = GURL(value->GetString());

    // This variable is required for metrics collection in order to distinguish
    // between the no canonical URL found and the invalid canonical URL found
    // cases. The |canonical_url| GURL cannot be relied upon to distinguish
    // between these cases because GURLs created with invalid URLs can be
    // constructed as empty GURLs.
    canonical_url_found = true;
  }

  if (!canonical_url_found) {
    // Log result if no canonical URL is found.
    UMA_HISTOGRAM_ENUMERATION(
        activity_services::kCanonicalURLResultHistogram,
        activity_services::FAILED_NO_CANONICAL_URL_DEFINED,
        activity_services::CANONICAL_URL_RESULT_COUNT);
  } else if (!canonical_url.is_valid()) {
    // Log result if an invalid canonical URL is found.
    UMA_HISTOGRAM_ENUMERATION(activity_services::kCanonicalURLResultHistogram,
                              activity_services::FAILED_CANONICAL_URL_INVALID,
                              activity_services::CANONICAL_URL_RESULT_COUNT);
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
    UMA_HISTOGRAM_ENUMERATION(activity_services::kCanonicalURLResultHistogram,
                              activity_services::FAILED_VISIBLE_URL_NOT_HTTPS,
                              activity_services::CANONICAL_URL_RESULT_COUNT);
    completion(GURL::EmptyGURL());
    return;
  }

  void (^javascript_completion)(const base::Value*) =
      ^(const base::Value* value) {
        GURL canonical_url = UrlFromValue(value);

        if (!canonical_url.is_empty())
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
