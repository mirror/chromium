// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/framed_browser_window.h"

#include <math.h>
#include <objc/runtime.h>
#include <stddef.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/sdk_forward_declarations.h"
#include "chrome/browser/global_keyboard_shortcuts_mac.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/browser_window_touch_bar.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/cocoa/nsgraphics_context_additions.h"
#import "ui/base/cocoa/nsview_additions.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

// Implementer's note: Moving the window controls is tricky. When altering the
// code, ensure that:
// - accessibility hit testing works
// - the accessibility hierarchy is correct
// - close/min in the background don't bring the window forward
// - rollover effects work correctly

namespace {

// Size of the gradient. Empirically determined so that the gradient looks
// like what the heuristic does when there are just a few tabs.
const CGFloat kWindowGradientHeight = 24.0;

}

@interface NSThemeFrame : NSView
- (void)_addKnownSubview:(NSView*)aView
              positioned:(NSWindowOrderingMode)place
              relativeTo:(NSView*)otherView;
- (CGFloat)_minXTitlebarWidgetInset;
- (CGFloat)_minYTitlebarButtonsOffset;
- (CGFloat)_titlebarHeight;
- (NSPoint)_fullScreenButtonOrigin;
- (void)_invalidateTitleCellSize;
- (void)_setContentView:(NSView*)contentView;
- (CGRect)contentRectForFrameRect:(CGRect)frameRect styleMask:(NSUInteger)style;
- (CGRect)frameRectForContentRect:(CGRect)contentRect
                        styleMask:(NSUInteger)style;
- (NSView*)fullScreenButton;
@end

@interface NSWindow (TenTwelveSDK)
@property(readonly)
    NSUserInterfaceLayoutDirection windowTitlebarLayoutDirection;
@end

@interface FramedBrowserWindow ()
@property(readonly) BOOL hasTabStrip;

- (void)childWindowsDidChange;
- (CGFloat)fullScreenButtonOriginAdjustment;

@end

@interface FramedBrowserWindowFrame : NSThemeFrame
@property(readonly) BOOL hasTabStrip;
@end

@implementation FramedBrowserWindowFrame

- (BOOL)hasTabStrip {
  return static_cast<FramedBrowserWindow*>(self.window).hasTabStrip;
}

// NSThemeFrame overrides.

- (CGFloat)_minXTitlebarWidgetInset {
  return self.hasTabStrip ? 11 : [super _minXTitlebarWidgetInset];
}

- (CGFloat)_minYTitlebarButtonsOffset {
  return self.hasTabStrip ? -12 : [super _minYTitlebarButtonsOffset];
}

- (CGFloat)_titlebarHeight {
  return self.hasTabStrip ? 40 : [super _titlebarHeight];
}

- (NSPoint)_fullScreenButtonOrigin {
  if (self.hasTabStrip) {
    CGFloat xAdjustment = [static_cast<FramedBrowserWindow*>(self.window)
        fullScreenButtonOriginAdjustment];
    NSSize fullScreenButtonSize = [self fullScreenButton].frame.size;
    return NSMakePoint(
        NSMaxX(self.bounds) - fullScreenButtonSize.width - 9 - xAdjustment,
        NSMaxY(self.bounds) - fullScreenButtonSize.height - 9);
  } else {
    return [super _fullScreenButtonOrigin];
  }
}

// The AppKit implementation does the same thing but also checks that [self
// class] == [NSThemeFrame class]. Yup. We could override -class, but that
// causes -[NSWindow setStyleMask:] to recreate the frame view because
// +frameViewClassForStyleMask: returns FramedBrowserWindowFrame and it thinks
// that the current frame view is the wrong class for the new style mask.
- (BOOL)_shouldFlipTrafficLightsForRTL {
  return self.window.windowTitlebarLayoutDirection ==
         NSUserInterfaceLayoutDirectionRightToLeft;
}

// On 10.9, we need to ensure that the content view is below the window
// buttons. This is already the case on more recent OSs.
- (void)_setContentView:(NSView*)contentView {
  if (base::mac::IsAtMostOS10_9()) {
    [self addSubview:contentView positioned:NSWindowBelow relativeTo:nil];
  } else {
    [super _setContentView:contentView];
  }
}

- (CGRect)contentRectForFrameRect:(CGRect)frameRect
                        styleMask:(NSUInteger)style {
  return self.hasTabStrip
             ? frameRect
             : [super contentRectForFrameRect:frameRect styleMask:style];
}

- (CGRect)frameRectForContentRect:(CGRect)contentRect
                        styleMask:(NSUInteger)style {
  return self.hasTabStrip
             ? contentRect
             : [super frameRectForContentRect:contentRect styleMask:style];
}

// NSObject overrides.

// NSWindow becomes restless unless its frame view's class is on a whitelist.
// See -[NSWindow _usesCustomDrawing].
- (BOOL)isMemberOfClass:(Class)aClass {
  return aClass == [NSThemeFrame class] || [super isMemberOfClass:aClass];
}

@end

@implementation FramedBrowserWindow

@synthesize hasTabStrip = hasTabStrip_;

+ (Class)frameViewClassForStyleMask:(NSUInteger)windowStyle {
  return [FramedBrowserWindowFrame class];
}

+ (CGFloat)browserFrameViewPaintHeight {
  return chrome::ShouldUseFullSizeContentView() ? chrome::kTabStripHeight
                                                : 60.0;
}

- (void)setStyleMask:(NSUInteger)styleMask {
  if (styleMaskLock_)
    return;
  [super setStyleMask:styleMask];
}

- (id)initWithContentRect:(NSRect)contentRect
              hasTabStrip:(BOOL)hasTabStrip{
  NSUInteger styleMask = NSTitledWindowMask |
                         NSClosableWindowMask |
                         NSMiniaturizableWindowMask |
                         NSResizableWindowMask |
                         NSTexturedBackgroundWindowMask;
  bool shouldUseFullSizeContentView =
      chrome::ShouldUseFullSizeContentView() && hasTabStrip;
  if (shouldUseFullSizeContentView) {
    styleMask |= NSFullSizeContentViewWindowMask;
  }

  if ((self = [super initWithContentRect:contentRect
                               styleMask:styleMask
                                 backing:NSBackingStoreBuffered
                                   defer:YES])) {
    // The 10.6 fullscreen code copies the title to a different window, which
    // will assert if it's nil.
    [self setTitle:@""];

    // The following two calls fix http://crbug.com/25684 by preventing the
    // window from recalculating the border thickness as the window is
    // resized.
    // This was causing the window tint to change for the default system theme
    // when the window was being resized.
    [self setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
    [self setContentBorderThickness:kWindowGradientHeight forEdge:NSMaxYEdge];

    hasTabStrip_ = hasTabStrip;
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)setShouldHideTitle:(BOOL)flag {
  shouldHideTitle_ = flag;
}

- (void)setStyleMaskLock:(BOOL)lock {
  styleMaskLock_ = lock;
}

- (BOOL)_isTitleHidden {
  return shouldHideTitle_;
}

// This method is called whenever a window is moved in order to ensure it fits
// on the screen.  We cannot always handle resizes without breaking, so we
// prevent frame constraining in those cases.
- (NSRect)constrainFrameRect:(NSRect)frame toScreen:(NSScreen*)screen {
  // Do not constrain the frame rect if our delegate says no.  In this case,
  // return the original (unconstrained) frame.
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(shouldConstrainFrameRect)] &&
      ![delegate shouldConstrainFrameRect])
    return frame;

  return [super constrainFrameRect:frame toScreen:screen];
}

- (CGFloat)fullScreenButtonOriginAdjustment {
  if (hasTabStrip_) {
    // If there is a profile avatar icon present, shift the button over by its
    // width and some padding. The new avatar button is displayed to the right
    // of the fullscreen icon, so it doesn't need to be shifted.
    BrowserWindowController* bwc =
        base::mac::ObjCCastStrict<BrowserWindowController>(
            [self windowController]);
    if ([bwc shouldShowAvatar] && ![bwc shouldUseNewAvatarButton]) {
      NSView* avatarButton = [[bwc avatarButtonController] view];
      return NSWidth([avatarButton frame]) - 3;
    }
  }
  return 0;
}

+ (BOOL)drawWindowThemeInDirtyRect:(NSRect)dirtyRect
                           forView:(NSView*)view
                            bounds:(NSRect)bounds
              forceBlackBackground:(BOOL)forceBlackBackground {
  const ui::ThemeProvider* themeProvider = [[view window] themeProvider];
  if (!themeProvider)
    return NO;

  ThemedWindowStyle windowStyle = [[view window] themedWindowStyle];

  // Devtools windows don't get themed.
  if (windowStyle & THEMED_DEVTOOLS)
    return NO;

  BOOL active = [[view window] isMainWindow];
  BOOL incognito = windowStyle & THEMED_INCOGNITO;
  BOOL popup = windowStyle & THEMED_POPUP;

  // Find a theme image.
  NSColor* themeImageColor = nil;
  if (!popup) {
    int themeImageID;
    if (active && incognito)
      themeImageID = IDR_THEME_FRAME_INCOGNITO;
    else if (active && !incognito)
      themeImageID = IDR_THEME_FRAME;
    else if (!active && incognito)
      themeImageID = IDR_THEME_FRAME_INCOGNITO_INACTIVE;
    else
      themeImageID = IDR_THEME_FRAME_INACTIVE;
    if (themeProvider->HasCustomImage(IDR_THEME_FRAME))
      themeImageColor = themeProvider->GetNSImageColorNamed(themeImageID);
  }

  BOOL themed = NO;
  if (themeImageColor) {
    // Default to replacing any existing pixels with the theme image, but if
    // asked paint black first and blend the theme with black.
    NSCompositingOperation operation = NSCompositeCopy;
    if (forceBlackBackground) {
      [[NSColor blackColor] set];
      NSRectFill(dirtyRect);
      operation = NSCompositeSourceOver;
    }

    NSPoint position = [[view window] themeImagePositionForAlignment:
        THEME_IMAGE_ALIGN_WITH_FRAME];
    [[NSGraphicsContext currentContext] cr_setPatternPhase:position
                                                   forView:view];

    [themeImageColor set];
    NSRectFillUsingOperation(dirtyRect, operation);
    themed = YES;
  }

  // Check to see if we have an overlay image.
  NSImage* overlayImage = nil;
  if (themeProvider->HasCustomImage(IDR_THEME_FRAME_OVERLAY) && !incognito &&
      !popup) {
    overlayImage = themeProvider->
        GetNSImageNamed(active ? IDR_THEME_FRAME_OVERLAY :
                                 IDR_THEME_FRAME_OVERLAY_INACTIVE);
  }

  if (overlayImage) {
    // Anchor to top-left and don't scale.
    NSPoint position = [[view window] themeImagePositionForAlignment:
        THEME_IMAGE_ALIGN_WITH_FRAME];
    position = [view convertPoint:position fromView:nil];
    NSSize overlaySize = [overlayImage size];
    NSRect imageFrame = NSMakeRect(0, 0, overlaySize.width, overlaySize.height);
    [overlayImage drawAtPoint:NSMakePoint(position.x,
                                          position.y - overlaySize.height)
                     fromRect:imageFrame
                    operation:NSCompositeSourceOver
                     fraction:1.0];
  }

  return themed;
}

- (NSTouchBar*)makeTouchBar {
  BrowserWindowController* bwc =
      base::mac::ObjCCastStrict<BrowserWindowController>(
          [self windowController]);
  return [[bwc browserWindowTouchBar] makeTouchBar];
}

- (NSColor*)titleColor {
  const ui::ThemeProvider* themeProvider = [self themeProvider];
  if (!themeProvider)
    return [NSColor windowFrameTextColor];

  ThemedWindowStyle windowStyle = [self themedWindowStyle];
  BOOL incognito = windowStyle & THEMED_INCOGNITO;

  if (incognito)
    return [NSColor whiteColor];
  else
    return [NSColor windowFrameTextColor];
}

- (void)addChildWindow:(NSWindow*)childWindow
               ordered:(NSWindowOrderingMode)orderingMode {
  [super addChildWindow:childWindow ordered:orderingMode];
  [self childWindowsDidChange];
}

- (void)removeChildWindow:(NSWindow*)childWindow {
  [super removeChildWindow:childWindow];
  [self childWindowsDidChange];
}

- (void)childWindowsDidChange {
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(childWindowsDidChange)])
    [delegate childWindowsDidChange];
}

@end
