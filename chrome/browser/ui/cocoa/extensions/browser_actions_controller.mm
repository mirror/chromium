// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"

#include <string>

#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/extensions/browser_action_button.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_container_view.h"
#import "chrome/browser/ui/cocoa/extensions/extension_popup_controller.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#import "chrome/browser/ui/cocoa/menu_button.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_delegate.h"
#include "grit/theme_resources.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"

NSString* const kBrowserActionVisibilityChangedNotification =
    @"BrowserActionVisibilityChangedNotification";

namespace {

const CGFloat kAnimationDuration = 0.2;

const CGFloat kChevronWidth = 18;

// Since the container is the maximum height of the toolbar, we have
// to move the buttons up by this amount in order to have them look
// vertically centered within the toolbar.
const CGFloat kBrowserActionOriginYOffset = 5.0;

// How far to inset from the bottom of the view to get the top border
// of the popup 2px below the bottom of the Omnibox.
const CGFloat kBrowserActionBubbleYOffset = 3.0;

}  // namespace

@interface BrowserActionsController(Private)

// Creates and adds a view for the given |action| at |index|.
- (void)addViewForAction:(ToolbarActionViewController*)action
               withIndex:(NSUInteger)index;

// Removes the view for the given |action| from the ccontainer.
- (void)removeViewForAction:(ToolbarActionViewController*)action;

// Removes views for all actions.
- (void)removeAllViews;

// Redraws the BrowserActionsContainerView and updates the button order to match
// the order in the ToolbarActionsBar.
- (void)redraw;

// Resizes the container to the specified |width|, and animates according to
// the ToolbarActionsBar.
- (void)resizeContainerToWidth:(CGFloat)width;

// Sets the container to be either hidden or visible based on whether there are
// any actions to show.
// Returns whether the container is visible.
- (BOOL)updateContainerVisibility;

// During container resizing, buttons become more transparent as they are pushed
// off the screen. This method updates each button's opacity determined by the
// position of the button.
- (void)updateButtonOpacity;

// Returns the existing button associated with the given id; nil if it cannot be
// found.
- (BrowserActionButton*)buttonForId:(const std::string&)id;

// Notification handlers for events registered by the class.

// Updates each button's opacity, the cursor rects and chevron position.
- (void)containerFrameChanged:(NSNotification*)notification;

// Hides the chevron and unhides every hidden button so that dragging the
// container out smoothly shows the Browser Action buttons.
- (void)containerDragStart:(NSNotification*)notification;

// Determines which buttons need to be hidden based on the new size, hides them
// and updates the chevron overflow menu. Also fires a notification to let the
// toolbar know that the drag has finished.
- (void)containerDragFinished:(NSNotification*)notification;

// Adjusts the position of the surrounding action buttons depending on where the
// button is within the container.
- (void)actionButtonDragging:(NSNotification*)notification;

// Updates the position of the Browser Actions within the container. This fires
// when _any_ Browser Action button is done dragging to keep all open windows in
// sync visually.
- (void)actionButtonDragFinished:(NSNotification*)notification;

// Moves the given button both visually and within the toolbar model to the
// specified index.
- (void)moveButton:(BrowserActionButton*)button
           toIndex:(NSUInteger)index;

// Handles clicks for BrowserActionButtons.
- (BOOL)browserActionClicked:(BrowserActionButton*)button;

// The reason |frame| is specified in these chevron functions is because the
// container may be animating and the end frame of the animation should be
// passed instead of the current frame (which may be off and cause the chevron
// to jump at the end of its animation).

// Shows the overflow chevron button depending on whether there are any hidden
// extensions within the frame given.
- (void)showChevronIfNecessaryInFrame:(NSRect)frame;

// Moves the chevron to its correct position within |frame|.
- (void)updateChevronPositionInFrame:(NSRect)frame;

// Shows or hides the chevron in the given |frame|.
- (void)setChevronHidden:(BOOL)hidden
                 inFrame:(NSRect)frame;

// Handles when a menu item within the chevron overflow menu is selected.
- (void)chevronItemSelected:(id)menuItem;

// Updates the container's grippy cursor based on the number of hidden buttons.
- (void)updateGrippyCursors;

@end

namespace {

// A bridge between the ToolbarActionsBar and the BrowserActionsController.
class ToolbarActionsBarBridge : public ToolbarActionsBarDelegate {
 public:
  explicit ToolbarActionsBarBridge(BrowserActionsController* controller);
  ~ToolbarActionsBarBridge() override;

  BrowserActionsController* controller_for_test() { return controller_; }

 private:
  // ToolbarActionsBarDelegate:
  void AddViewForAction(ToolbarActionViewController* action,
                        size_t index) override;
  void RemoveViewForAction(ToolbarActionViewController* action) override;
  void RemoveAllViews() override;
  void Redraw(bool order_changed) override;
  void ResizeAndAnimate(gfx::Tween::Type tween_type,
                        int target_width,
                        bool suppress_chevron) override;
  void SetChevronVisibility(bool chevron_visible) override;
  int GetWidth() const override;
  bool IsAnimating() const override;
  void StopAnimating() override;
  int GetChevronWidth() const override;
  bool IsPopupRunning() const override;

  // The owning BrowserActionsController; weak.
  BrowserActionsController* controller_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarActionsBarBridge);
};

ToolbarActionsBarBridge::ToolbarActionsBarBridge(
    BrowserActionsController* controller)
    : controller_(controller) {
}

ToolbarActionsBarBridge::~ToolbarActionsBarBridge() {
}

void ToolbarActionsBarBridge::AddViewForAction(
    ToolbarActionViewController* action,
    size_t index) {
  [controller_ addViewForAction:action
                      withIndex:index];
}

void ToolbarActionsBarBridge::RemoveViewForAction(
    ToolbarActionViewController* action) {
  [controller_ removeViewForAction:action];
}

void ToolbarActionsBarBridge::RemoveAllViews() {
  [controller_ removeAllViews];
}

void ToolbarActionsBarBridge::Redraw(bool order_changed) {
  [controller_ redraw];
}

void ToolbarActionsBarBridge::ResizeAndAnimate(gfx::Tween::Type tween_type,
                                               int target_width,
                                               bool suppress_chevron) {
  [controller_ resizeContainerToWidth:target_width];
}

void ToolbarActionsBarBridge::SetChevronVisibility(bool chevron_visible) {
  [controller_ setChevronHidden:!chevron_visible
                        inFrame:[[controller_ containerView] frame]];
}

int ToolbarActionsBarBridge::GetWidth() const {
  return NSWidth([[controller_ containerView] frame]);
}

bool ToolbarActionsBarBridge::IsAnimating() const {
  return false;
}

void ToolbarActionsBarBridge::StopAnimating() {
}

int ToolbarActionsBarBridge::GetChevronWidth() const {
  return kChevronWidth;
}

bool ToolbarActionsBarBridge::IsPopupRunning() const {
  return [ExtensionPopupController popup] != nil;
}

}  // namespace

@implementation BrowserActionsController

@synthesize containerView = containerView_;

#pragma mark -
#pragma mark Public Methods

- (id)initWithBrowser:(Browser*)browser
        containerView:(BrowserActionsContainerView*)container {
  DCHECK(browser && container);

  if ((self = [super init])) {
    browser_ = browser;

    toolbarActionsBarBridge_.reset(new ToolbarActionsBarBridge(self));
    toolbarActionsBar_.reset(
        new ToolbarActionsBar(toolbarActionsBarBridge_.get(), browser_, false));

    containerView_ = container;
    [containerView_ setPostsFrameChangedNotifications:YES];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(containerFrameChanged:)
               name:NSViewFrameDidChangeNotification
             object:containerView_];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(containerDragStart:)
               name:kBrowserActionGrippyDragStartedNotification
             object:containerView_];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(containerDragFinished:)
               name:kBrowserActionGrippyDragFinishedNotification
             object:containerView_];
    // Listen for a finished drag from any button to make sure each open window
    // stays in sync.
    [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(actionButtonDragFinished:)
             name:kBrowserActionButtonDragEndNotification
           object:nil];

    suppressChevron_ = NO;
    chevronAnimation_.reset([[NSViewAnimation alloc] init]);
    [chevronAnimation_ gtm_setDuration:kAnimationDuration
                             eventMask:NSLeftMouseUpMask];
    [chevronAnimation_ setAnimationBlockingMode:NSAnimationNonblocking];

    buttons_.reset([[NSMutableArray alloc] init]);
    toolbarActionsBar_->CreateActions();
    [self showChevronIfNecessaryInFrame:[containerView_ frame]];
    [self updateGrippyCursors];
    [container setResizable:YES];
  }

  return self;
}

- (void)dealloc {
  // Explicitly destroy the ToolbarActionsBar so all buttons get removed with a
  // valid BrowserActionsController, and so we can verify state before
  // destruction.
  toolbarActionsBar_->DeleteActions();
  toolbarActionsBar_.reset();
  DCHECK_EQ(0u, [buttons_ count]);
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)update {
  for (BrowserActionButton* button in buttons_.get())
    [button updateState];
}

- (NSUInteger)buttonCount {
  return [buttons_ count];
}

- (NSUInteger)visibleButtonCount {
  NSUInteger visibleCount = 0;
  for (BrowserActionButton* button in buttons_.get())
    visibleCount += [button superview] == containerView_;
  return visibleCount;
}

- (CGFloat)savedWidth {
  return toolbarActionsBar_->GetPreferredSize().width();
}

- (NSPoint)popupPointForId:(const std::string&)id {
  BrowserActionButton* button = [self buttonForId:id];
  if (!button)
    return NSZeroPoint;

  NSRect bounds;
  NSButton* referenceButton = button;
  if ([button superview] != containerView_) {
    bounds = [chevronMenuButton_ bounds];
    referenceButton = chevronMenuButton_.get();
  } else {
    bounds = [button convertRect:[button frameAfterAnimation]
                        fromView:[button superview]];
  }

  // Anchor point just above the center of the bottom.
  DCHECK([referenceButton isFlipped]);
  NSPoint anchor = NSMakePoint(NSMidX(bounds),
                               NSMaxY(bounds) - kBrowserActionBubbleYOffset);
  // Convert the point to the container view's frame, and adjust for animation.
  NSPoint anchorInContainer =
      [containerView_ convertPoint:anchor fromView:referenceButton];
  anchorInContainer.x -= NSMinX([containerView_ frame]) -
      NSMinX([containerView_ animationEndFrame]);

  return [containerView_ convertPoint:anchorInContainer toView:nil];
}

- (BOOL)chevronIsHidden {
  if (!chevronMenuButton_.get())
    return YES;

  if (![chevronAnimation_ isAnimating])
    return [chevronMenuButton_ isHidden];

  DCHECK([[chevronAnimation_ viewAnimations] count] > 0);

  // The chevron is animating in or out. Determine which one and have the return
  // value reflect where the animation is headed.
  NSString* effect = [[[chevronAnimation_ viewAnimations] objectAtIndex:0]
      valueForKey:NSViewAnimationEffectKey];
  if (effect == NSViewAnimationFadeInEffect) {
    return NO;
  } else if (effect == NSViewAnimationFadeOutEffect) {
    return YES;
  }

  NOTREACHED();
  return YES;
}

- (content::WebContents*)currentWebContents {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

#pragma mark -
#pragma mark NSMenuDelegate

- (void)menuNeedsUpdate:(NSMenu*)menu {
  [menu removeAllItems];

  // See menu_button.h for documentation on why this is needed.
  [menu addItemWithTitle:@"" action:nil keyEquivalent:@""];

  NSUInteger iconCount = toolbarActionsBar_->GetIconCount();
  NSRange hiddenButtonRange =
      NSMakeRange(iconCount, [buttons_ count] - iconCount);
  for (BrowserActionButton* button in
           [buttons_ subarrayWithRange:hiddenButtonRange]) {
    NSString* name =
        base::SysUTF16ToNSString([button viewController]->GetActionName());
    NSMenuItem* item =
        [menu addItemWithTitle:name
                        action:@selector(chevronItemSelected:)
                 keyEquivalent:@""];
    [item setRepresentedObject:button];
    [item setImage:[button compositedImage]];
    [item setTarget:self];
    [item setEnabled:[button isEnabled]];
  }
}

#pragma mark -
#pragma mark Private Methods

- (void)addViewForAction:(ToolbarActionViewController*)action
               withIndex:(NSUInteger)index {
  NSRect buttonFrame = NSMakeRect(NSMaxX([containerView_ bounds]),
                                  kBrowserActionOriginYOffset,
                                  ToolbarActionsBar::IconWidth(false),
                                  ToolbarActionsBar::IconHeight());
  BrowserActionButton* newButton =
      [[[BrowserActionButton alloc]
         initWithFrame:buttonFrame
        viewController:action
            controller:self] autorelease];
  [newButton setTarget:self];
  [newButton setAction:@selector(browserActionClicked:)];
  [buttons_ insertObject:newButton atIndex:index];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(actionButtonDragging:)
             name:kBrowserActionButtonDraggingNotification
           object:newButton];
}

- (void)redraw {
  if (![self updateContainerVisibility])
    return;  // Container is hidden; no need to update.

  std::vector<ToolbarActionViewController*> toolbar_actions =
      toolbarActionsBar_->toolbar_actions();
  for (NSUInteger i = 0; i < [buttons_ count]; ++i) {
    if ([[buttons_ objectAtIndex:i] viewController] != toolbar_actions[i]) {
      size_t j = i + 1;
      while (toolbar_actions[i] != [[buttons_ objectAtIndex:j] viewController])
        ++j;
      [buttons_ exchangeObjectAtIndex:i withObjectAtIndex: j];
    }
  }

  [self showChevronIfNecessaryInFrame:[containerView_ frame]];
  for (NSUInteger i = 0; i < [buttons_ count]; ++i) {
    if (![[buttons_ objectAtIndex:i] isBeingDragged])
      [self moveButton:[buttons_ objectAtIndex:i] toIndex:i];
  }
}

- (void)removeViewForAction:(ToolbarActionViewController*)action {
  BrowserActionButton* button = [self buttonForId:action->GetId()];

  [button removeFromSuperview];
  [button onRemoved];
  [buttons_ removeObject:button];
}

- (void)removeAllViews {
  for (BrowserActionButton* button in buttons_.get()) {
    [button removeFromSuperview];
    [button onRemoved];
  }
  [buttons_ removeAllObjects];
}

- (void)resizeContainerToWidth:(CGFloat)width {
  BOOL animate = !toolbarActionsBar_->suppress_animation();
  [self updateContainerVisibility];
  [containerView_ setMaxWidth:
      toolbarActionsBar_->IconCountToWidth([self buttonCount])];
  [containerView_ resizeToWidth:width
                        animate:animate];
  NSRect frame = animate ? [containerView_ animationEndFrame] :
                           [containerView_ frame];

  [self showChevronIfNecessaryInFrame:frame];

  [containerView_ setNeedsDisplay:YES];

  if (!animate) {
    [[NSNotificationCenter defaultCenter]
        postNotificationName:kBrowserActionVisibilityChangedNotification
                      object:self];
  }
  [self redraw];
}

- (BOOL)updateContainerVisibility {
  BOOL hidden = [buttons_ count] == 0;
  if ([containerView_ isHidden] != hidden)
    [containerView_ setHidden:hidden];
  return !hidden;
}

- (void)updateButtonOpacity {
  for (BrowserActionButton* button in buttons_.get()) {
    NSRect buttonFrame = [button frame];
    if (NSContainsRect([containerView_ bounds], buttonFrame)) {
      if ([button alphaValue] != 1.0)
        [button setAlphaValue:1.0];

      continue;
    }
    CGFloat intersectionWidth =
        NSWidth(NSIntersectionRect([containerView_ bounds], buttonFrame));
    CGFloat alpha = std::max(static_cast<CGFloat>(0.0),
                             intersectionWidth / NSWidth(buttonFrame));
    [button setAlphaValue:alpha];
    [button setNeedsDisplay:YES];
  }
}

- (BrowserActionButton*)buttonForId:(const std::string&)id {
  for (BrowserActionButton* button in buttons_.get()) {
    if ([button viewController]->GetId() == id)
      return button;
  }
  return nil;
}

- (void)containerFrameChanged:(NSNotification*)notification {
  [self updateButtonOpacity];
  [[containerView_ window] invalidateCursorRectsForView:containerView_];
  [self updateChevronPositionInFrame:[containerView_ frame]];
}

- (void)containerDragStart:(NSNotification*)notification {
  [self setChevronHidden:YES inFrame:[containerView_ frame]];
  for (BrowserActionButton* button in buttons_.get()) {
    if ([button superview] != containerView_) {
      [button setAlphaValue:1.0];
      [containerView_ addSubview:button];
    }
  }
}

- (void)containerDragFinished:(NSNotification*)notification {
  for (BrowserActionButton* button in buttons_.get()) {
    NSRect buttonFrame = [button frame];
    if (NSContainsRect([containerView_ bounds], buttonFrame))
      continue;

    CGFloat intersectionWidth =
        NSWidth(NSIntersectionRect([containerView_ bounds], buttonFrame));
    // Pad the threshold by 5 pixels in order to have the buttons hide more
    // easily.
    if (([containerView_ grippyPinned] && intersectionWidth > 0) ||
        (intersectionWidth <= (NSWidth(buttonFrame) / 2) + 5.0)) {
      [button setAlphaValue:0.0];
      [button removeFromSuperview];
    }
  }

  toolbarActionsBar_->OnResizeComplete(
      toolbarActionsBar_->IconCountToWidth([self visibleButtonCount]));

  [self updateGrippyCursors];
  [self resizeContainerToWidth:toolbarActionsBar_->GetPreferredSize().width()];
}

- (void)actionButtonDragging:(NSNotification*)notification {
  suppressChevron_ = YES;
  if (![self chevronIsHidden])
    [self setChevronHidden:YES inFrame:[containerView_ frame]];

  // Determine what index the dragged button should lie in, alter the model and
  // reposition the buttons.
  CGFloat dragThreshold = ToolbarActionsBar::IconWidth(false) / 2;
  BrowserActionButton* draggedButton = [notification object];
  NSRect draggedButtonFrame = [draggedButton frame];

  NSUInteger index = 0;
  for (BrowserActionButton* button in buttons_.get()) {
    CGFloat intersectionWidth =
        NSWidth(NSIntersectionRect(draggedButtonFrame, [button frame]));

    if (intersectionWidth > dragThreshold && button != draggedButton &&
        ![button isAnimating] && index < [self visibleButtonCount]) {
      toolbarActionsBar_->OnDragDrop(
          [buttons_ indexOfObject:draggedButton],
          index,
          ToolbarActionsBar::DRAG_TO_SAME);
      return;
    }
    ++index;
  }
}

- (void)actionButtonDragFinished:(NSNotification*)notification {
  suppressChevron_ = NO;
  [self redraw];
}

- (void)moveButton:(BrowserActionButton*)button
           toIndex:(NSUInteger)index {
  const ToolbarActionsBar::PlatformSettings& platformSettings =
      toolbarActionsBar_->platform_settings();
  CGFloat xOffset = platformSettings.left_padding +
      (index * ToolbarActionsBar::IconWidth(true));
  NSRect buttonFrame = [button frame];
  buttonFrame.origin.x = xOffset;
  [button setFrame:buttonFrame
           animate:!toolbarActionsBar_->suppress_animation()];

  if (index < toolbarActionsBar_->GetIconCount()) {
    // Make sure the button is within the visible container.
    if ([button superview] != containerView_) {
      // We add the subview under the sibling views so that when it "slides in",
      // it does so under its neighbors.
      [containerView_ addSubview:button
                      positioned:NSWindowBelow
                      relativeTo:nil];
    }
    // We need to set the alpha value in case the container has resized.
    [button setAlphaValue:1.0];
  } else if ([button superview] == containerView_) {
    [button removeFromSuperview];
    [button setAlphaValue:0.0];
  }
}

- (BOOL)browserActionClicked:(BrowserActionButton*)button {
  return [button viewController]->ExecuteAction(true);
}

- (void)showChevronIfNecessaryInFrame:(NSRect)frame {
  bool hidden = suppressChevron_ ||
      toolbarActionsBar_->GetIconCount() == [self buttonCount];
  [self setChevronHidden:hidden inFrame:frame];
}

- (void)updateChevronPositionInFrame:(NSRect)frame {
  CGFloat xPos = NSWidth(frame) - kChevronWidth;
  NSRect buttonFrame = NSMakeRect(xPos,
                                  kBrowserActionOriginYOffset,
                                  kChevronWidth,
                                  ToolbarActionsBar::IconHeight());
  [chevronAnimation_ stopAnimation];
  [chevronMenuButton_ setFrame:buttonFrame];
}

- (void)setChevronHidden:(BOOL)hidden
                 inFrame:(NSRect)frame {
  if (hidden == [self chevronIsHidden])
    return;

  if (!chevronMenuButton_.get()) {
    chevronMenuButton_.reset([[MenuButton alloc] init]);
    [chevronMenuButton_ setOpenMenuOnClick:YES];
    [chevronMenuButton_ setBordered:NO];
    [chevronMenuButton_ setShowsBorderOnlyWhileMouseInside:YES];

    [[chevronMenuButton_ cell] setImageID:IDR_BROWSER_ACTIONS_OVERFLOW
                           forButtonState:image_button_cell::kDefaultState];
    [[chevronMenuButton_ cell] setImageID:IDR_BROWSER_ACTIONS_OVERFLOW_H
                           forButtonState:image_button_cell::kHoverState];
    [[chevronMenuButton_ cell] setImageID:IDR_BROWSER_ACTIONS_OVERFLOW_P
                           forButtonState:image_button_cell::kPressedState];

    overflowMenu_.reset([[NSMenu alloc] initWithTitle:@""]);
    [overflowMenu_ setAutoenablesItems:NO];
    [overflowMenu_ setDelegate:self];
    [chevronMenuButton_ setAttachedMenu:overflowMenu_];

    [containerView_ addSubview:chevronMenuButton_];
  }

  [self updateChevronPositionInFrame:frame];

  // Stop any running animation.
  [chevronAnimation_ stopAnimation];

  if (toolbarActionsBar_->suppress_animation()) {
    [chevronMenuButton_ setHidden:hidden];
    return;
  }

  NSString* animationEffect;
  if (hidden) {
    animationEffect = NSViewAnimationFadeOutEffect;
  } else {
    [chevronMenuButton_ setHidden:NO];
    animationEffect = NSViewAnimationFadeInEffect;
  }
  NSDictionary* animationDictionary = @{
      NSViewAnimationTargetKey : chevronMenuButton_.get(),
      NSViewAnimationEffectKey : animationEffect
  };
  [chevronAnimation_ setViewAnimations:
      [NSArray arrayWithObject:animationDictionary]];
  [chevronAnimation_ startAnimation];
}

- (void)chevronItemSelected:(id)menuItem {
  [self browserActionClicked:[menuItem representedObject]];
}

- (void)updateGrippyCursors {
  [containerView_
      setCanDragLeft:toolbarActionsBar_->GetIconCount() != [buttons_ count]];
  [containerView_ setCanDragRight:[self visibleButtonCount] > 0];
  [[containerView_ window] invalidateCursorRectsForView:containerView_];
}

#pragma mark -
#pragma mark Testing Methods

- (BrowserActionButton*)buttonWithIndex:(NSUInteger)index {
  return index < [buttons_ count] ? [buttons_ objectAtIndex:index] : nil;
}

- (ToolbarActionsBar*)toolbarActionsBar {
  return toolbarActionsBar_.get();
}

+ (BrowserActionsController*)fromToolbarActionsBarDelegate:
    (ToolbarActionsBarDelegate*)delegate {
  return static_cast<ToolbarActionsBarBridge*>(delegate)->controller_for_test();
}

@end
