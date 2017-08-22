// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_HOVER_BUTTON_
#define UI_BASE_COCOA_HOVER_BUTTON_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/tracking_area.h"
#import "ui/base/ui_base_export.h"

// A button that changes when you hover over it and click it.
UI_BASE_EXPORT
@interface HoverButton : NSButton {
 @protected
  // Enumeration of the hover states that the close button can be in at any one
  // time. The button cannot be in more than one hover state at a time.
  enum HoverState {
    kHoverStateNone = 0,
    kHoverStateMouseOver = 1,
    kHoverStateMouseDown = 2
  };

  HoverState hoverState_;

 @private
  // Tracking area for button mouseover states. Nil if not enabled.
  ui::ScopedCrTrackingArea trackingArea_;
  BOOL mouseDown_;
}

@property(nonatomic) HoverState hoverState;
@property(nonatomic) BOOL trackingEnabled;

// An NSRect in the parent view's coordinate space representing the area which
// the buton should be sensitive to for hover and hit testing. Default value is
// NSZeroRect, which makes the hitbox equal to the view's frame. Subclasses may
// override and return a different value. The MD download shelf uses this in
// fullscreen to extend the close button's hitbox to the edges of the window.
@property(readonly, nonatomic) NSRect hitbox;

// Common initialization called from initWithFrame: and awakeFromNib.
// Subclassers should call [super commonInit].
- (void)commonInit;

// Text that would be announced by screen readers.
- (void)setAccessibilityTitle:(NSString*)accessibilityTitle;

// Checks to see whether the mouse is in the button's bounds and update
// the image in case it gets out of sync.  This occurs to the close button
// when you close a tab so the tab to the left of it takes its place, and
// drag the button without moving the mouse before you press the button down.
- (void)checkImageState;

@end

#endif  // UI_BASE_COCOA_HOVER_BUTTON_
