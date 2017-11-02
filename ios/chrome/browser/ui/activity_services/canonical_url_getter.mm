// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/canonical_url_getter.h"

#include "base/mac/bind_objc_block.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#import "ios/chrome/browser/ui/activity_services/canonical_url_getter_delegate.h"
#import "ios/web/public/web_state/web_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Script to access the canonical URL from a web page.
char kCanonicalURLScript[] =
    "var linkNode = document.querySelector(\"link[rel='canonical']\");"
    "linkNode ? linkNode.getAttribute(\"href\") : \"\";";
}

@implementation CanonicalURLGetter
@synthesize delegate = _delegate;

// Converts a |value| to a GURL. Returns an empty GURL if |value| is not a valid
// URL.
+ (GURL)URLFromValue:(const base::Value*)value {
  GURL canonicalURL = GURL();
  if (value->is_string()) {
    canonicalURL = GURL(value->GetString());
  }
  return canonicalURL.is_valid() ? canonicalURL : GURL();
}

- (void)retrieveCanonicalURLForWebState:(web::WebState*)webState {
  BOOL visibleURLIsCryptographic =
      webState->GetVisibleURL().SchemeIsCryptographic();

  __weak __typeof(self) weakSelf = self;
  void (^completionCallback)(const base::Value*) = ^void(
      const base::Value* value) {
    GURL canonicalURL = [CanonicalURLGetter URLFromValue:value];

    // In case where the visible URL is HTTPS and the canonical is HTTP, prevent
    // the downgrade by passing an empty GURL.
    if (visibleURLIsCryptographic && !canonicalURL.SchemeIsCryptographic()) {
      canonicalURL = GURL();
    }
    [weakSelf.delegate canonicalURLGetter:weakSelf
                  didRetrieveCanonicalURL:canonicalURL];
  };

  webState->ExecuteJavaScript(base::UTF8ToUTF16(kCanonicalURLScript),
                              base::BindBlockArc(completionCallback));
}

@end
