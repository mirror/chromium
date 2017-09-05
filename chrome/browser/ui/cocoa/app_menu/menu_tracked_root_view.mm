// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/app_menu/menu_tracked_root_view.h"

#include <Carbon/Carbon.h>

@interface NSView (MenuTrackedButton)
- (void)doHighlight:(BOOL)highlight;
- (void)performClick:(id)sender;
@end

@implementation MenuTrackedRootView {
  NSView* activeView_;
}

@synthesize menuItem = menuItem_;

- (void)drawRect:(NSRect)dirtyRect {
}

- (void)mouseUp:(NSEvent*)theEvent {
  [[menuItem_ menu] cancelTracking];
}

- (void)keyDown:(NSEvent*)theEvent {
  NSView* oldActiveView = activeView_;
  int code = [theEvent keyCode];
  if (code == kVK_Tab) {
    code = ([theEvent modifierFlags] & NSShiftKeyMask) ? kVK_LeftArrow
                                                       : kVK_RightArrow;
  }
  switch (code) {
    case kVK_UpArrow:
    case kVK_DownArrow:
      activeView_ = nil;
      break;
    case kVK_RightArrow:
      activeView_ = [(activeView_ ? activeView_ : self)nextValidKeyView];
      break;
    case kVK_LeftArrow:
      activeView_ = [(activeView_ ? activeView_ : self)previousValidKeyView];
      break;
    case kVK_Space:
    case kVK_Return:
      if ([activeView_ respondsToSelector:@selector(performClick:)])
        [activeView_ performClick:self];
      break;
  }
  if (oldActiveView != activeView_) {
    if ([oldActiveView respondsToSelector:@selector(doHighlight:)])
      [oldActiveView doHighlight:NO];
    if ([activeView_ respondsToSelector:@selector(doHighlight:)])
      [activeView_ doHighlight:YES];
  }
  [super keyDown:theEvent];
}

- (NSRect)focusRingMaskBounds {
  // The focus ring always clips badly. Show focus via highlight.
  return NSZeroRect;
}

- (BOOL)isEditable {
  return NO;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

@end
