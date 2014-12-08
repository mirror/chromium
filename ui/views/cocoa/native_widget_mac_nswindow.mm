// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/views/cocoa/native_widget_mac_nswindow.h"

#include "base/mac/foundation_util.h"
#import "ui/views/cocoa/views_nswindow_delegate.h"

@interface NativeWidgetMacNSWindow ()
- (ViewsNSWindowDelegate*)viewsNSWindowDelegate;
@end

@implementation NativeWidgetMacNSWindow

- (ViewsNSWindowDelegate*)viewsNSWindowDelegate {
  return base::mac::ObjCCastStrict<ViewsNSWindowDelegate>([self delegate]);
}

// Override canBecome{Key,Main}Window to always return YES, otherwise Windows
// with a styleMask of NSBorderlessWindowMask default to NO.
- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

// Override display, since this is the first opportunity Cocoa gives to detect
// a visibility change in some cases. For example, restoring from the dock first
// calls -[NSWindow display] before any NSWindowDelegate functions and before
// ordering the window (and without actually calling -[NSWindow deminiaturize]).
// By notifying the delegate that a display is about to occur, it can apply a
// correct visibility state, before [super display] requests a draw of the
// contentView. -[NSWindow isVisible] can still report NO at this point, so this
// gives the delegate time to apply correct visibility before the draw occurs.
- (void)display {
  [[self viewsNSWindowDelegate] onWindowWillDisplay];
  [super display];
}

// Override window order functions to intercept other visibility changes. This
// is needed in addition to the -[NSWindow display] override because Cocoa
// hardly ever calls display, and reports -[NSWindow isVisible] incorrectly
// when ordering in a window for the first time.
- (void)orderWindow:(NSWindowOrderingMode)orderingMode
         relativeTo:(NSInteger)otherWindowNumber {
  [[self viewsNSWindowDelegate] onWindowOrderWillChange:orderingMode];
  [super orderWindow:orderingMode relativeTo:otherWindowNumber];
  [[self viewsNSWindowDelegate] onWindowOrderChanged:nil];
}

@end
