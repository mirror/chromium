// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_placeholder_navigation_info.h"

#import <objc/runtime.h>

#include "base/logging.h"

// The address of this static variable is used to set and get the is-placeholder
// flag value from a WKNavigation object. A WKNavigation is marked as a
// placeholder if it is created by |loadPlaceholderInWebViewForURL|. Placeholder
// navigations are used to create placeholder WKBackForwardListItems that
// correspond to Native View or WebUI URLs.
const void* kPlaceholderNavigationKey = &kPlaceholderNavigationKey;

@interface CRWPlaceholderNavigationInfo (Private)

// Initializes a new instance wrapping |handler|.
- (instancetype)initWithCompletionHandler:(ProceduralBlock)handler;

@end

@implementation CRWPlaceholderNavigationInfo {
  ProceduralBlock _completionHandler;
}

+ (void)setInfoForNavigation:(nonnull WKNavigation*)navigation
           completionHandler:(ProceduralBlock)handler {
  DCHECK(navigation);
  CRWPlaceholderNavigationInfo* info =
      [[CRWPlaceholderNavigationInfo alloc] initWithCompletionHandler:handler];
  objc_setAssociatedObject(navigation, &kPlaceholderNavigationKey, info,
                           OBJC_ASSOCIATION_RETAIN);
}

+ (nullable CRWPlaceholderNavigationInfo*)getInfoForNavigation:
    (nullable WKNavigation*)navigation {
  return objc_getAssociatedObject(navigation, &kPlaceholderNavigationKey);
}

- (instancetype)initWithCompletionHandler:(ProceduralBlock)handler {
  self = [super init];
  if (self) {
    _completionHandler = handler;
  }
  return self;
}

- (void)runCompletionHandler {
  _completionHandler();
}

@end
