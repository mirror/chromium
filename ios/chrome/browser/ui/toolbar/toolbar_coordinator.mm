// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/toolbar_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation TToolbarCoordinator
@synthesize tabModel = _tabModel;
@synthesize view = _view;

- (void)updateButtonVisibility {
}

- (void)browserStateDestroyed {
}

- (void)selectedTabChanged {
}

- (void)updateToolbarState {
}

- (void)setShareButtonEnabled:(BOOL)enabled {
}

- (void)showPrerenderingAnimation {
}

- (BOOL)isOmniboxFirstResponder {
  return NO;
}

- (BOOL)showingOmniboxPopup{
  return NO;
}

- (void)focusOmnibox {
}

- (void)cancelOmniboxEdit {
}

- (void)focusFakebox {
}

- (void)onFakeboxBlur {
}

- (void)onFakeboxAnimationComplete {
}

- (void)currentPageLoadStarted {
}

@end
