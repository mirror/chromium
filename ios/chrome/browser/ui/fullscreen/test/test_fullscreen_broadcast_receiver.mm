// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_broadcast_receiver.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation TestFullscreenBroadcastReceiver
@synthesize scrollOffset = _scrollOffset;
@synthesize dragging = _dragging;
@synthesize scrolling = _scrolling;
@synthesize toolbarHeight = _toolbarHeight;
@end
