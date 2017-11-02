// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_GETTER_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_GETTER_H_

#import <Foundation/Foundation.h>

namespace web {
class WebState;
}

@protocol CanonicalURLGetterDelegate;

// CanonicalURLGetter accesses the canonical URL from within a web page
// represented by a WebState.
@interface CanonicalURLGetter : NSObject

// The delegate which is notified when the canonical URL is retrieved. Must be
// set before |retrieveCanonicalURLForWebState:| in order for the canonical URL
// to be accessed.
@property(nonatomic, weak) id<CanonicalURLGetterDelegate> delegate;

// Retrieves the canonical URL in the web page represented by |webState|. This
// method is asynchronous, so the URL is returned through a callback on
// |delegate|.
// There are a few special cases:
// 1. If there is more than one canonical URL defined, the first one
// (found through a depth-first search of the DOM) is given to the |delegate|.
// 2. If either no canonical URL is found, or the canonical URL is invalid, an
// empty GURL is given to |delegate|.
// 3. Last, no HTTPS downgrades are allowed. So, if the visible URL is HTTPS but
// the canonical URL is HTTP, an empty GURL is given to the |delegate|.
- (void)retrieveCanonicalURLForWebState:(web::WebState*)webState;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_GETTER_H_
