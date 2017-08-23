// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/hover_button.h"

#import "ui/events/test/cocoa_test_event_utils.h"
#import "ui/gfx/test/ui_cocoa_test_helper.h"

@interface TestHoverButton : HoverButton
@property(readwrite, nonatomic) NSRect hitbox;
@end

@implementation TestHoverButton
@synthesize hitbox = hitbox_;

- (void)setHitbox:(NSRect)hitbox {
  hitbox_ = hitbox;
  [self updateTrackingAreas];
}
@end

@interface HoverButtonTestTarget : NSObject
@property(nonatomic, copy) void (^cb)(id);
@end

@implementation HoverButtonTestTarget
@synthesize cb = cb_;

- (IBAction)action:(id)sender {
  if (cb_)
    cb_(sender);
}
@end

namespace {

class HoverButtonTest : public ui::CocoaTest {
 public:
  HoverButtonTest() {
    NSRect frame = NSMakeRect(0, 0, 20, 20);
    base::scoped_nsobject<TestHoverButton> button(
        [[TestHoverButton alloc] initWithFrame:frame]);
    button_ = button;
    target_.reset([[HoverButtonTestTarget alloc] init]);
    button_.target = target_;
    button_.action = @selector(action:);
    [[test_window() contentView] addSubview:button_];
  }

 protected:
  void HoverAndExpect(HoverState hoverState) {
    EXPECT_EQ(kHoverStateNone, button_.hoverState);
    [button_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
    EXPECT_EQ(hoverState, button_.hoverState);
    [button_ mouseExited:cocoa_test_event_utils::ExitEvent()];
    EXPECT_EQ(kHoverStateNone, button_.hoverState);
  }

  bool HandleMouseDown(NSEvent* mouseDownEvent) {
    __block bool action_sent = false;
    target_.get().cb = ^(id sender) {
      action_sent = true;
      EXPECT_EQ(kHoverStateMouseDown, button_.hoverState);
    };
    [NSApp sendEvent:mouseDownEvent];
    target_.get().cb = nil;
    return action_sent;
  }

  TestHoverButton* button_;  // Weak, owned by test_window().
  base::scoped_nsobject<HoverButtonTestTarget> target_;
};

}  // namespace

TEST_VIEW(HoverButtonTest, button_)

TEST_F(HoverButtonTest, Hover) {
  EXPECT_EQ(kHoverStateNone, button_.hoverState);

  // Default
  HoverAndExpect(kHoverStateMouseOver);

  // Tracking disabled
  button_.trackingEnabled = NO;
  HoverAndExpect(kHoverStateNone);
  button_.trackingEnabled = YES;

  // Button disabled
  button_.enabled = NO;
  HoverAndExpect(kHoverStateNone);
  button_.enabled = YES;

  // Back to normal
  HoverAndExpect(kHoverStateMouseOver);
}

TEST_F(HoverButtonTest, Click) {
  EXPECT_EQ(kHoverStateNone, button_.hoverState);
  const auto click = cocoa_test_event_utils::MouseClickInView(button_, 1);

  [NSApp postEvent:click.second atStart:YES];
  EXPECT_TRUE(HandleMouseDown(click.first));

  button_.enabled = NO;
  EXPECT_FALSE(HandleMouseDown(click.first));

  EXPECT_EQ(kHoverStateNone, button_.hoverState);
}

TEST_F(HoverButtonTest, CustomHitbox) {
  NSRect hitbox = button_.frame;
  hitbox.size.width += 10;

  NSPoint insideHitPoint =
      NSMakePoint(NSMaxX(button_.frame) + 5, NSMidY(button_.frame));
  NSPoint outsideHitPoint =
      NSMakePoint(insideHitPoint.x + 10, insideHitPoint.y);

  EXPECT_FALSE(NSPointInRect(insideHitPoint, button_.trackingAreas[0].rect));
  EXPECT_FALSE(NSPointInRect(outsideHitPoint, button_.trackingAreas[0].rect));
  EXPECT_NE(button_, [button_ hitTest:insideHitPoint]);
  EXPECT_EQ(nil, [button_ hitTest:outsideHitPoint]);

  button_.hitbox = hitbox;
  EXPECT_TRUE(NSPointInRect(insideHitPoint, button_.trackingAreas[0].rect));
  EXPECT_FALSE(NSPointInRect(outsideHitPoint, button_.trackingAreas[0].rect));
  EXPECT_EQ(button_, [button_ hitTest:insideHitPoint]);
  EXPECT_EQ(nil, [button_ hitTest:outsideHitPoint]);

  button_.hitbox = NSZeroRect;
  EXPECT_FALSE(NSPointInRect(insideHitPoint, button_.trackingAreas[0].rect));
  EXPECT_FALSE(NSPointInRect(outsideHitPoint, button_.trackingAreas[0].rect));
  EXPECT_NE(button_, [button_ hitTest:insideHitPoint]);
  EXPECT_EQ(nil, [button_ hitTest:outsideHitPoint]);
}
