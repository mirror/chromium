// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"

#include <stdint.h>

#include <utility>

#include "base/feature_list.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/devtools/devtools_window.h"
#import "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/cocoa/fullscreen_placeholder_view.h"
#include "chrome/browser/ui/cocoa/separate_fullscreen_window.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/web_textfield_touch_bar_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_within_tab_helper.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/theme_resources.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/cocoa/animation_utils.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/scrollbar_size.h"

using content::WebContents;
using content::WebContentsObserver;

// FullscreenObserver is used by TabContentsController to monitor for the
// showing/destruction of fullscreen render widgets.  When notified,
// TabContentsController will alter its child view hierarchy to either embed a
// fullscreen render widget view or restore the normal WebContentsView render
// view.  The embedded fullscreen render widget will fill the user's screen in
// the case where TabContentsController's NSView is a subview of a browser
// window that has been toggled into fullscreen mode (e.g., via
// FullscreenController).
class FullscreenObserver : public WebContentsObserver {
 public:
  explicit FullscreenObserver(TabContentsController* controller)
      : controller_(controller) {}

  void Observe(content::WebContents* new_web_contents) {
    WebContentsObserver::Observe(new_web_contents);
  }

  WebContents* web_contents() const {
    return WebContentsObserver::web_contents();
  }

  void DidShowFullscreenWidget() override {
    [controller_ toggleFullscreenWidget:YES];
  }

  void DidDestroyFullscreenWidget() override {
    [controller_ toggleFullscreenWidget:NO];
  }

  void DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                     bool will_cause_resize) override {
    [controller_ toggleFullscreenWidget:entered_fullscreen];
  }

 private:
  TabContentsController* const controller_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenObserver);
};

@interface TabContentsController (TabContentsContainerViewDelegate)
- (BOOL)contentsInFullscreenCaptureMode;

// Computes and returns the frame to use for the scaling view within the
// TabContentsContainerView.
- (NSRect)frameForScalingViewIn:(NSView*)container;

// Computes and returns the bounds to use for the scaling view within the
// TabContentsContainerView. In the normal case, the bounds size will be
// equivalent to frame size. In 'AutoEmbedFullscreen mode', the bounds size is
// set to the video capture resolution, which is almost always different from
// the frame size. See 'AutoEmbedFullscreen mode' in header file comments.
- (NSRect)boundsForScalingViewIn:(NSView*)container;

// Returns YES if the content view should be resized.
- (BOOL)shouldResizeContentView;

// Returns YES if the content view is inside a popup.
- (BOOL)isPopup;

@end

// An NSView with special-case handling for when the contents view does not
// expand to fill the entire tab contents area. See 'AutoEmbedFullscreen mode'
// in header file comments.
//
// This view creates and manages its own subview, a "scaling" view, which acts
// as a container for the contents view. It allows the rendering size of the
// contents view to be something different than its normal on-screen display
// size.
@interface TabContentsContainerView : NSView {
 @private
  TabContentsController* delegate_;  // weak
}
- (void)ensureContentsVisible:(NSView*)contentsView;
- (void)updateBackgroundColorFromWindowTheme:(NSWindow*)window;
@end

@implementation TabContentsContainerView

- (id)initWithDelegate:(TabContentsController*)delegate {
  if ((self = [super initWithFrame:NSZeroRect])) {
    delegate_ = delegate;

    ScopedCAActionDisabler disabler;
    base::scoped_nsobject<CALayer> layer([[CALayer alloc] init]);
    [self setLayer:layer];
    [self setWantsLayer:YES];

    // Create and add the scaling view.
    [self addSubview:[[NSView alloc] initWithFrame:NSZeroRect]];
  }
  return self;
}

// Called by the delegate during dealloc to invalidate the pointer held by this
// view.
- (void)delegateDestroyed {
  delegate_ = nil;
}

// Returns the scaling view, which is the child of TabContentsContainerView.
- (NSView*)scalingView {
  return [[self subviews] objectAtIndex:0];
}

// Return the contents view, which is the child of the scaling view; or nil if
// there isn't one attached.
- (NSView*)contentsView {
  NSArray* const scalingViewSubviews = [[self scalingView] subviews];
  if ([scalingViewSubviews count] == 0)
    return nil;  // No contents view attached.
  return [scalingViewSubviews objectAtIndex:0];
}

// Override auto-resizing logic to query the delegate for the exact frame and
// bounds to use for the contents view, and then perform manual layout of the
// scaling view (child view) and contents view (grandchild view).
- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  ScopedCAActionDisabler disabler;

  NSView* const contentsView = [self contentsView];
  if (!contentsView)
    return;  // No contents view attached yet.

  // TODO(spqchan): The popup check is a temporary solution to fix the
  // regression described in crbug.com/604288. This method doesn't really affect
  // fullscreen if the content is inside a normal browser window, but would
  // cause a flash fullscreen widget to blow up if it's inside a popup.
  if (!delegate_ ||
      (![delegate_ shouldResizeContentView] && [delegate_ isPopup])) {
    return;
  }

  // Set the position and size of the scaling view within this container view.
  NSView* const scalingView = [self scalingView];
  [scalingView setFrame:[delegate_ frameForScalingViewIn:self]];
  const NSRect scalingViewBounds = [delegate_ boundsForScalingViewIn:self];
  [scalingView setBounds:scalingViewBounds];

  // Update the position and size of the contents view, if it has changed.
  if (!NSEqualRects(scalingViewBounds, [contentsView frame]))
    [contentsView setFrame:scalingViewBounds];
}

// Update the background layer's color whenever the view needs to repaint.
- (void)setNeedsDisplayInRect:(NSRect)rect {
  [super setNeedsDisplayInRect:rect];
  [self updateBackgroundColorFromWindowTheme:[self window]];
}

- (void)ensureContentsVisible:(NSView*)contentsView {
  DCHECK(contentsView);

  // TabContentsContainerView performs manual layout.
  [contentsView setAutoresizingMask:NSViewNotSizable];

  NSView* const scalingView = [self scalingView];
  if ([[scalingView subviews] count] == 0) {
    [scalingView addSubview:contentsView];
  } else if ([[scalingView subviews] objectAtIndex:0] != contentsView) {
    [scalingView replaceSubview:[[scalingView subviews] objectAtIndex:0]
                           with:contentsView];
  }

  [self resizeSubviewsWithOldSize:[self bounds].size];
}

- (void)updateBackgroundColorFromWindowTheme:(NSWindow*)window {
  // This view is sometimes flashed into visibility (e.g, when closing
  // windows or opening new tabs), so ensure that the flash be the theme
  // background color in those cases.
  const ThemeProvider* theme = [window themeProvider];
  ThemedWindowStyle windowStyle = [window themedWindowStyle];
  if (!theme)
    return;

  // This logic and hard-coded color value are duplicated from the function
  // NTPResourceCache::CreateNewTabIncognitoCSS. This logic should exist in only
  // one location.
  // https://crbug.com/719236
  SkColor skBackgroundColor =
      theme->GetColor(ThemeProperties::COLOR_NTP_BACKGROUND);
  bool incognito = windowStyle & THEMED_INCOGNITO;
  if (incognito && !theme->HasCustomImage(IDR_THEME_NTP_BACKGROUND))
    skBackgroundColor = SkColorSetRGB(0x32, 0x32, 0x32);

  // If the page is in fullscreen tab capture mode, change the background color
  // to be a dark tint of the new tab page's background color.
  if ([delegate_ contentsInFullscreenCaptureMode]) {
    const int kBackgroundDivisor = 5;
    skBackgroundColor = skBackgroundColor = SkColorSetARGB(
        SkColorGetA(skBackgroundColor),
        SkColorGetR(skBackgroundColor) / kBackgroundDivisor,
        SkColorGetG(skBackgroundColor) / kBackgroundDivisor,
        SkColorGetB(skBackgroundColor) / kBackgroundDivisor);
  }

  ScopedCAActionDisabler disabler;
  base::ScopedCFTypeRef<CGColorRef> cgBackgroundColor(
      skia::CGColorCreateFromSkColor(skBackgroundColor));
  [[self layer] setBackgroundColor:cgBackgroundColor];
}

- (void)viewWillMoveToWindow:(NSWindow*)newWindow {
  [self updateBackgroundColorFromWindowTheme:newWindow];
}

- (ViewID)viewID {
  return VIEW_ID_TAB_CONTAINER;
}

- (BOOL)acceptsFirstResponder {
  NSView* const contentsView = [self contentsView];
  return contentsView && [contentsView acceptsFirstResponder];
}

// When receiving a click-to-focus in the solid color area surrounding the
// WebContents' native view, immediately transfer focus to WebContents' native
// view.
- (BOOL)becomeFirstResponder {
  if (![self acceptsFirstResponder])
    return NO;
  return [[self window] makeFirstResponder:[self contentsView]];
}

- (BOOL)canBecomeKeyView {
  return NO;  // Tab/Shift-Tab should focus the subview, not this view.
}

@end  // @implementation TabContentsContainerView

@interface TabContentsController (
    SeparateFullscreenWindowDelegate)<NSWindowDelegate>

- (NSView*)createScreenshotView;

- (NSWindow*)createSeparateWindowForTab:(content::WebContents*)separatedTab;

@end

@implementation TabContentsController
@synthesize webContents = contents_;
@synthesize blockFullscreenResize = blockFullscreenResize_;

- (id)initWithContents:(WebContents*)contents isPopup:(BOOL)popup {
  if ((self = [super initWithNibName:nil bundle:nil])) {
    fullscreenObserver_.reset(new FullscreenObserver(self));
    [self changeWebContents:contents];
    isPopup_ = popup;
    touchBarController_.reset([[WebTextfieldTouchBarController alloc]
        initWithTabContentsController:self]);
  }
  return self;
}

- (void)dealloc {
  [static_cast<TabContentsContainerView*>([self view]) delegateDestroyed];
  // Make sure the contents view has been removed from the container view to
  // allow objects to be released.
  [[self view] removeFromSuperview];
  [super dealloc];
}

- (void)loadView {
  base::scoped_nsobject<NSView> view(
      [[TabContentsContainerView alloc] initWithDelegate:self]);
  [view setAutoresizingMask:NSViewHeightSizable|NSViewWidthSizable];
  [self setView:view];
}

- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2)) {
  return [touchBarController_ makeTouchBar];
}

- (void)ensureContentsVisibleInSuperview:(NSView*)superview {
  if (!contents_)
    return;

  ScopedCAActionDisabler disabler;
  NSView* contentsNativeView;
  content::RenderWidgetHostView* const fullscreenView =
      isEmbeddingFullscreenWidget_ ?
      contents_->GetFullscreenRenderWidgetHostView() : NULL;
  if (fullscreenPlaceholderView_) {
    contentsNativeView = fullscreenPlaceholderView_;
  } else if (fullscreenView) {
    contentsNativeView = fullscreenView->GetNativeView();
  } else {
    isEmbeddingFullscreenWidget_ = NO;
    contentsNativeView = contents_->GetNativeView();
  }

  TabContentsContainerView* const contentsContainer =
      static_cast<TabContentsContainerView*>([self view]);
  [contentsContainer ensureContentsVisible:contentsNativeView];
  [contentsContainer setFrame:[superview bounds]];
  [superview addSubview:contentsContainer];
  [contentsContainer setNeedsDisplay:YES];
}

- (void)updateFullscreenWidgetFrame {
  // This should only apply if a fullscreen widget is embedded.
  if (!isEmbeddingFullscreenWidget_ || blockFullscreenResize_)
    return;

  content::RenderWidgetHostView* const fullscreenView =
      contents_->GetFullscreenRenderWidgetHostView();
  if (fullscreenView) {
    [static_cast<TabContentsContainerView*>([self view])
        ensureContentsVisible:fullscreenView->GetNativeView()];
  }
}

- (void)changeWebContents:(WebContents*)newContents {
  contents_ = newContents;
  fullscreenObserver_->Observe(contents_);
  isEmbeddingFullscreenWidget_ =
      contents_ && contents_->GetFullscreenRenderWidgetHostView();
}

// Returns YES if the tab represented by this controller is the front-most.
- (BOOL)isCurrentTab {
  // We're the current tab if we're in the view hierarchy, otherwise some other
  // tab is.
  return [[self view] superview] ? YES : NO;
}

- (void)willBecomeUnselectedTab {
  // The RWHV is ripped out of the view hierarchy on tab switches, so it never
  // formally resigns first responder status.  Handle this by explicitly sending
  // a Blur() message to the renderer, but only if the RWHV currently has focus.
  content::RenderViewHost* rvh = [self webContents]->GetRenderViewHost();
  if (rvh) {
    if (rvh->GetWidget()->GetView() &&
        rvh->GetWidget()->GetView()->HasFocus()) {
      rvh->GetWidget()->Blur();
      return;
    }
    WebContents* devtools = DevToolsWindow::GetInTabWebContents(
        [self webContents], NULL);
    if (devtools) {
      content::RenderViewHost* devtoolsView = devtools->GetRenderViewHost();
      if (devtoolsView && devtoolsView->GetWidget()->GetView() &&
          devtoolsView->GetWidget()->GetView()->HasFocus()) {
        devtoolsView->GetWidget()->Blur();
      }
    }
  }
}

- (void)willBecomeSelectedTab {
  // Do not explicitly call Focus() here, as the RWHV may not actually have
  // focus (for example, if the omnibox has focus instead).  The WebContents
  // logic will restore focus to the appropriate view.
}

- (void)tabDidChange:(WebContents*)updatedContents {
  // Calling setContentView: here removes any first responder status
  // the view may have, so avoid changing the view hierarchy unless
  // the view is different.
  if ([self webContents] != updatedContents) {
    [self changeWebContents:updatedContents];
    [self ensureContentsVisibleInSuperview:[[self view] superview]];
  }
}

- (void)toggleFullscreenWidget:(BOOL)enterFullscreen {
  isEmbeddingFullscreenWidget_ = enterFullscreen &&
      contents_ && contents_->GetFullscreenRenderWidgetHostView();
  if (base::FeatureList::IsEnabled(features::kContentFullscreen)) {
    if (enterFullscreen) {
      fullscreenPlaceholderView_ = [self createScreenshotView];
      separateFullscreenWindow_ = [self createSeparateWindowForTab:contents_];

      [separateFullscreenWindow_ makeKeyAndOrderFront:nil];
      [separateFullscreenWindow_ toggleFullScreen:nil];
    } else {
      [separateFullscreenWindow_ close];
    }
  }
  [self ensureContentsVisibleInSuperview:[[self view] superview]];
}

- (BOOL)contentsInFullscreenCaptureMode {
  // Note: Grab a known-valid WebContents pointer from |fullscreenObserver_|.
  content::WebContents* const wc = fullscreenObserver_->web_contents();
  if (!wc ||
      wc->GetCapturerCount() == 0 ||
      wc->GetPreferredSize().IsEmpty() ||
      !(isEmbeddingFullscreenWidget_ ||
        (wc->GetDelegate() &&
         wc->GetDelegate()->IsFullscreenForTabOrPending(wc)))) {
    return NO;
  }
  return YES;
}

- (NSRect)frameForScalingViewIn:(NSView*)container {
  NSRect bounds = [container bounds];
  bounds.origin = NSZeroPoint;

  // In most cases, the contents view is simply sized to fill the container
  // view's bounds. Only WebContentses that are in fullscreen mode and being
  // screen-captured will engage the special layout/sizing behavior.
  if (![self contentsInFullscreenCaptureMode])
    return bounds;

  // For 'AutoEmbedFullscreen mode', scale the view to fit within the container,
  // and center it.
  gfx::RectF rect(bounds);
  gfx::Size captureSize =
      fullscreenObserver_->web_contents()->GetPreferredSize();
  DCHECK(!captureSize.IsEmpty());
  const float x = captureSize.width() * rect.height();
  const float y = captureSize.height() * rect.width();
  if (y < x) {
    rect.ClampToCenteredSize(gfx::SizeF(rect.width(), y / captureSize.width()));
  } else {
    rect.ClampToCenteredSize(
        gfx::SizeF(x / captureSize.height(), rect.height()));
  }
  // Ensure the bounds align with pixel boundaries.
  return gfx::ToEnclosedRect(rect).ToCGRect();
}

- (NSRect)boundsForScalingViewIn:(NSView*)container {
  NSRect bounds;
  bounds.origin = NSZeroPoint;

  // When in 'AutoEmbedFullscreen mode' mode, the bounds size is set to the
  // video capture resolution. See header file for more details.
  if ([self contentsInFullscreenCaptureMode]) {
    content::WebContents* const wc = fullscreenObserver_->web_contents();
    gfx::Size captureSize = wc->GetPreferredSize();
    DCHECK(!captureSize.IsEmpty());
    bounds.size = captureSize.ToCGSize();
  } else {
    bounds.size = [container bounds].size;
  }
  return bounds;
}

- (BOOL)shouldResizeContentView {
  return !isEmbeddingFullscreenWidget_ || !blockFullscreenResize_;
}

- (BOOL)isPopup {
  return isPopup_;
}

- (WebTextfieldTouchBarController*)webTextfieldTouchBarController {
  return touchBarController_.get();
}

@end

@implementation TabContentsController (SeparateFullscreenWindowDelegate)

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  // Make the RenderWidgetHostViewCocoa the firstResponder for the
  // SeparateFullscreenWindow.
  contents_->Focus();
}

- (void)windowWillExitFullScreen:(NSNotification*)notification {
  // Remove the screenshot view so that the WebContentsViewCocoa is
  // retrieved and displayed again in the original window.
  fullscreenPlaceholderView_ = nil;
  [self ensureContentsVisibleInSuperview:[[self view] superview]];

  // When exiting through the title bar Exit Fullscreen Window button, the
  // WebContents must be notified of the change in fullscreen (like in
  // FullscreenController::HandleUserPressedEscape).
  contents_->ExitFullscreen(true);
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  // When exiting through the title bar Exit Fullscreen Window button, the
  // SeparateFullscreenWindow doesn't close, so make sure it's closed.
  [separateFullscreenWindow_ close];
  separateFullscreenWindow_ = nil;
}

- (NSView*)createScreenshotView {
  // Getting the current's window view and its boundaries.
  NSWindow* window = [contents_->GetNativeView() window];
  NSView* view = contents_->GetNativeView();
  NSRect windowFrame = window.frame;
  NSRect viewFrame = [view convertRect:view.bounds toView:nil];

  // Moving the origin from the lower-left corner to the upper-left corner of
  // the view and cropping out the scrollbar
  viewFrame.origin.y = NSHeight(windowFrame) - NSMaxY(viewFrame);
  viewFrame.size.width -= gfx::scrollbar_size();

  // Taking a screenshot of the view and creating the custom view to display
  CGImageRef windowScreenshot = (CGImageRef)[(id)CGWindowListCreateImage(
      CGRectZero, kCGWindowListOptionIncludingWindow, [window windowNumber],
      kCGWindowImageBoundsIgnoreFraming) autorelease];
  CGImageRef viewScreenshot = (CGImageRef)[(id)CGImageCreateWithImageInRect(
      windowScreenshot, [window convertRectToBacking:viewFrame]) autorelease];
  FullscreenPlaceholderView* screenshotView =
      [[[FullscreenPlaceholderView alloc] initWithFrame:[[self view] bounds]
                                                  image:viewScreenshot]
          autorelease];
  screenshotView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

  return screenshotView;
}

// Creates a new window with the tab without detaching it from its source
// window.
- (NSWindow*)createSeparateWindowForTab:(WebContents*)separatedTab {
  DCHECK(separatedTab->GetNativeView());

  NSView* separatedTabView = separatedTab->GetNativeView();
  NSWindow* sourceWindow = [separatedTabView window];
  NSRect windowRect =
      [separatedTabView convertRect:[separatedTabView bounds] toView:nil];
  SeparateFullscreenWindow* separateWindow = [[SeparateFullscreenWindow alloc]
      initWithContentRect:[sourceWindow convertRectToScreen:windowRect]
                styleMask:NSResizableWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO];

  [separateWindow setDelegate:self];
  [[separateWindow contentView] addSubview:separatedTabView];
  [separateWindow
      setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];

  // Make TabContentsContainerView the first responder now as
  // WebContentsViewCocoa is now in a separate window.
  [sourceWindow makeFirstResponder:[self view]];

  return separateWindow;
}
@end
