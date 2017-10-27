// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_placeholder_navigation_info.h"

#import <objc/runtime.h>

#include "base/logging.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// The address of this static variable is used to set and get the is-placeholder
// flag value from a WKNavigation object. A WKNavigation is marked as a
// placeholder if it is created by |loadPlaceholderInWebViewForURL|. Placeholder
// navigations are used to create placeholder WKBackForwardListItems that
// correspond to Native View or WebUI URLs.
const void* kPlaceholderNavigationKey = &kPlaceholderNavigationKey;

@interface CRWPlaceholderNavigationInfo ()

// Initializes a new instance wrapping |handler|.
- (instancetype)initWithURL:(const GURL&)url
          completionHandler:(PlaceholderNavigationCompletion)handler
    NS_DESIGNATED_INITIALIZER;

@end

@implementation CRWPlaceholderNavigationInfo {
  GURL _url;
  PlaceholderNavigationCompletion _completionHandler;
}

@synthesize url = _url;

+ (instancetype)createForNavigation:(nonnull WKNavigation*)navigation
                              toURL:(const GURL&)url
              withCompletionHandler:(PlaceholderNavigationCompletion)handler {
  DCHECK(navigation);
  CRWPlaceholderNavigationInfo* info =
      [[CRWPlaceholderNavigationInfo alloc] initWithURL:url
                                      completionHandler:handler];
  objc_setAssociatedObject(navigation, &kPlaceholderNavigationKey, info,
                           OBJC_ASSOCIATION_RETAIN);
  return info;
}

+ (nullable CRWPlaceholderNavigationInfo*)infoForNavigation:
    (nullable WKNavigation*)navigation {
  return objc_getAssociatedObject(navigation, &kPlaceholderNavigationKey);
}

- (instancetype)initWithURL:(const GURL&)url
          completionHandler:(PlaceholderNavigationCompletion)handler {
  self = [super init];
  if (self) {
    _url = url;
    _completionHandler = [handler copy];
  }
  return self;
}

- (void)runCompletionHandler {
  _completionHandler(_url);
}

@end
