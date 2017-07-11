// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_popup_base_view_cocoa.h"

#include "chrome/browser/ui/autofill/autofill_popup_view_delegate.h"
#include "chrome/browser/ui/autofill/popup_constants.h"
#include "ui/base/cocoa/window_size_constants.h"

@implementation AutofillPopupBaseViewCocoa

#pragma mark -
#pragma mark Colors

- (NSColor*)backgroundColor {
  return [NSColor whiteColor];
}

- (NSColor*)borderColor {
  return [NSColor colorForControlTint:[NSColor currentControlTint]];
}

- (NSColor*)highlightColor {
  return [NSColor selectedControlColor];
}

- (NSColor*)nameColor {
  return [NSColor blackColor];
}

- (NSColor*)separatorColor {
  return [NSColor colorWithCalibratedWhite:220 / 255.0 alpha:1];
}

- (NSColor*)subtextColor {
  // Represents #646464.
  return [NSColor colorWithCalibratedRed:100.0 / 255.0
                                   green:100.0 / 255.0
                                    blue:100.0 / 255.0
                                   alpha:1.0];
}

#pragma mark -
#pragma mark Public methods

- (id)initWithDelegate:(autofill::AutofillPopupViewDelegate*)delegate
                 frame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self)
    popup_delegate_ = delegate;

  return self;
}

- (void)delegateDestroyed {
  popup_delegate_ = NULL;
}

- (void)drawSeparatorWithBounds:(NSRect)bounds {
  [[self separatorColor] set];
  [NSBezierPath fillRect:bounds];
}

// A slight optimization for drawing:
// https://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/CocoaViewsGuide/Optimizing/Optimizing.html
- (BOOL)isOpaque {
  return YES;
}

- (BOOL)isFlipped {
  // Flipped so that it's easier to share controller logic with other OSes.
  return YES;
}

- (void)drawBackgroundAndBorder {
  // The inset is needed since the border is centered on the |path|.
  // TODO(isherman): We should consider using asset-based drawing for the
  // border, creating simple bitmaps for the view's border and background, and
  // drawing them using NSDrawNinePartImage().
  CGFloat inset = autofill::kPopupBorderThickness / 2.0;
  NSRect borderRect = NSInsetRect([self bounds], inset, inset);
  NSBezierPath* path = [NSBezierPath bezierPathWithRect:borderRect];
  [[self backgroundColor] setFill];
  [path fill];
  [path setLineWidth:autofill::kPopupBorderThickness];
  [[self borderColor] setStroke];
  [path stroke];
}

- (void)mouseUp:(NSEvent*)theEvent {
  // If the view is in the process of being destroyed, abort.
  if (!popup_delegate_)
    return;

  // Only accept single-click.
  if ([theEvent clickCount] > 1)
    return;

  NSPoint location = [self convertPoint:[theEvent locationInWindow]
                               fromView:nil];

  if (NSPointInRect(location, [self bounds])) {
    popup_delegate_->SetSelectionAtPoint(
        gfx::Point(NSPointToCGPoint(location)));
    popup_delegate_->AcceptSelectedLine();
  }
}

- (void)mouseMoved:(NSEvent*)theEvent {
  // If the view is in the process of being destroyed, abort.
  if (!popup_delegate_)
    return;

  NSPoint location = [self convertPoint:[theEvent locationInWindow]
                               fromView:nil];

  popup_delegate_->SetSelectionAtPoint(gfx::Point(NSPointToCGPoint(location)));
}

- (void)mouseDragged:(NSEvent*)theEvent {
  [self mouseMoved:theEvent];
}

- (void)mouseExited:(NSEvent*)theEvent {
  // If the view is in the process of being destroyed, abort.
  if (!popup_delegate_)
    return;

  popup_delegate_->SelectionCleared();
}

#pragma mark -
#pragma mark Private methods:

// Returns the full bounds needed by the content, which may exceed the available
// vertical space (see getClippedPopupBounds).
- (NSRect)getFullPopupBounds {
  NSRect frame = NSRectFromCGRect(popup_delegate_->popup_bounds().ToCGRect());

  // Flip the y-origin back into Cocoa-land. The controller's platform-neutral
  // coordinate space places the origin at the top-left of the first screen
  // (e.g. 300 from the top), whereas Cocoa's coordinate space expects the
  // origin to be at the bottom-left of this same screen (e.g. 1200 from the
  // bottom). We calculate the new y-origin of the bottom-left corner of the
  // popup, by taking the height of the screen (e.g. 1600) and subtracting
  // what's equivalent to the initial y-origin + the height of the popup
  // (e.g. 300 + 100).
  NSScreen* screen = [[NSScreen screens] firstObject];
  // For a |popupFrame| that was 300 from the top and 100 high on a 1600 high
  // screen, this formula is 1600 - 400, placing the bottom corner 1200 from
  // the bottom edge of the screen.
  frame.origin.y = NSMaxY([screen frame]) - NSMaxY(frame);
  return frame;
}

// Returns the bounds of the popup that should be displayed, which are basically
// the bounds of the popup, clipped if there is not enough available vertical
// space.
- (NSRect)getClippedPopupBounds {
  NSRect clippedPopupBounds = [self getFullPopupBounds];

  // The y-origin of the popup may be off-screen (e.g. -300, or 300 below the
  // bottom edge). If this happens, it is corrected to be at the application
  // window's bottom edge. Remember that the y axis starts at the bottom of the
  // screen and the values count up.
  NSWindow* appWindow = [popup_delegate_->container_view() window];
  CGFloat appWindowBottomEdge = NSMinY([appWindow frame]);
  if (clippedPopupBounds.origin.y < appWindowBottomEdge) {
    clippedPopupBounds.origin.y = appWindowBottomEdge;

    // The height also needs to be adjusted to reflect the new origin. It is
    // equal to the distance between the top of the popup and the bottom edge
    // of the application window.

    // Distance between the top of the screen and the application window.
    CGFloat windowTopEdgeDistance =
        NSMaxY([[[NSScreen screens] firstObject] frame]) -
        NSMaxY([appWindow frame]);
    // The distance between the top of the application window and the top of the
    // popup.
    CGFloat popupTopDistance =
        popup_delegate_->popup_bounds().y() - windowTopEdgeDistance;
    clippedPopupBounds.size.height =
        [appWindow frame].size.height - popupTopDistance;
  }
  return clippedPopupBounds;
}

#pragma mark -
#pragma mark Messages from AutofillPopupViewBridge:

- (void)updateBoundsAndRedrawPopup {
  [[[self superview] window] setFrame:[self getClippedPopupBounds] display:YES];

  [self setNeedsDisplay:YES];
}

- (void)showPopup {
  NSRect clippedPopupBounds = [self getClippedPopupBounds];
  // The window contains a scroll view, and both are the same size, which is
  // may be clipped at the application window's bottom edge (see
  // getClippedPopupBounds).
  NSWindow* window =
      [[NSWindow alloc] initWithContentRect:clippedPopupBounds
                                  styleMask:NSBorderlessWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO];
  NSScrollView* scrollView =
      [[NSScrollView alloc] initWithFrame:clippedPopupBounds];
  // Configure the scroller to have no visible border.
  [scrollView setBorderType:NSNoBorder];

  // The |window| contains the |scrollView|, which contains |self|, the full
  // popup view (which is not clipped and may be longer than |scrollView|).
  [self setFrame:[self getFullPopupBounds]];
  [scrollView setDocumentView:self];
  [window setContentView:scrollView];

  // Telling Cocoa that the window is opaque enables some drawing optimizations.
  [window setOpaque:YES];

  [self updateBoundsAndRedrawPopup];
  [[popup_delegate_->container_view() window] addChildWindow:window
                                                     ordered:NSWindowAbove];
}

- (void)hidePopup {
  // Remove the child window before closing, otherwise it can mess up
  // display ordering.
  NSWindow* window = [self window];
  [[window parentWindow] removeChildWindow:window];
  [window close];
}

@end
