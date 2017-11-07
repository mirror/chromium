// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/canonical_url_retriever.h"

#include "base/mac/bind_objc_block.h"
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
char kCanonicalURLScript[] =
    "(function() {"
    "  var linkNode = document.querySelector(\"link[rel='canonical']\");"
    "  linkNode ? linkNode.getAttribute(\"href\") : \"\";"
    "})()";

// Converts a |value| to a GURL. Returns an empty GURL if |value| is not a valid
// URL.
GURL UrlFromValue(const base::Value* value) {
  GURL canonicalURL = GURL();
  if (value && value->is_string()) {
    canonicalURL = GURL(value->GetString());
  }
  return canonicalURL.is_valid() ? canonicalURL : GURL();
}
}  // namespace

namespace activity_services {
void RetrieveCanonicalUrl(web::WebState* web_state,
                          ProceduralBlockWithURL completion) {
  bool visible_url_is_cryptographic =
      web_state->GetVisibleURL().SchemeIsCryptographic();

  void (^javascript_completion)(const base::Value*) =
      ^(const base::Value* value) {
        GURL canonical_url = UrlFromValue(value);

        // In case where the visible URL is HTTPS and the canonical is HTTP,
        // prevent the downgrade by passing an empty GURL.
        if (visible_url_is_cryptographic &&
            !canonical_url.SchemeIsCryptographic()) {
          canonical_url = GURL();
        }
        completion(canonical_url);
      };

  web_state->ExecuteJavaScript(base::UTF8ToUTF16(kCanonicalURLScript),
                               base::BindBlockArc(javascript_completion));
}
}  // namespace activity_services
