// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_RETRIEVER_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_RETRIEVER_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/procedural_block_types.h"

namespace web {
class WebState;
}

namespace activity_services {

// Retrieves the canonical URL in the web page represented by |web_state|.
// This method is asynchronous and the URL is returned by calling the
// |completion| block.
// There are a few special cases:
// 1. If there is more than one canonical URL defined, the first one
// (found through a depth-first search of the DOM) is given to |completion|.
// 2. If either no canonical URL is found, or the canonical URL is invalid, an
// empty GURL is given to |completion|.
// 3. Last, no HTTPS downgrades are allowed. So, if the visible URL is HTTPS
// but the canonical URL is HTTP, an empty GURL is given to |completion|.
void RetrieveCanonicalUrl(web::WebState* web_state,
                          ProceduralBlockWithURL completion);
}  // namespace activity_services

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_RETRIEVER_H_
