// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/advanced_keys_view.h"

#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#import "remoting/ios/app/remoting_theme.h"

#include "remoting/client/input/keyboard_interpreter.h"
#include "ui/events/keycodes/dom/dom_code.h"

using ModifierFlag = remoting::KeyboardInterpreter::ModifierFlag;

static const int kViewHeight = 30.f;

#pragma mark - AdvancedKeyButton

@interface AdvancedKeyButton : MDCFlatButton

@property(nonatomic) uint32_t keycode;
@property(nonatomic) ModifierFlag modifier;

@end

@implementation AdvancedKeyButton

@synthesize keycode = _keycode;
@synthesize modifier = _modifier;

@end

#pragma mark - AdvancedKeysView

@implementation AdvancedKeysView {
  UIScrollView* _scrollView;
  NSArray<AdvancedKeyButton*>* _buttons;
}

#pragma mark UIView

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    self.autoresizingMask = UIViewAutoresizingFlexibleHeight;
    self.backgroundColor = RemotingTheme.advancedKeysViewBackgroundColor;

    ModifierFlag modifierNone = ModifierFlag::MODIFIER_NONE;
    [self initScrollViewWithButtons:@[
      [self keyButtonWithText:@"esc"
                      domCode:ui::DomCode::ESCAPE
                     modifier:modifierNone],
      [self keyButtonWithText:@"tab"
                      domCode:ui::DomCode::TAB
                     modifier:modifierNone],
      [self keyButtonWithText:@"ctrl"
                      domCode:ui::DomCode::CONTROL_LEFT
                     modifier:remoting::KeyboardInterpreter::MODIFIER_CTRL],
      [self keyButtonWithText:@"alt"
                      domCode:ui::DomCode::ALT_LEFT
                     modifier:remoting::KeyboardInterpreter::MODIFIER_ALT],
      [self keyButtonWithText:@"⌘"
                      domCode:ui::DomCode::META_LEFT
                     modifier:modifierNone],
      [self keyButtonWithText:@"del"
                      domCode:ui::DomCode::DEL
                     modifier:modifierNone],
      [self keyButtonWithText:@"↑"
                      domCode:ui::DomCode::ARROW_UP
                     modifier:modifierNone],
      [self keyButtonWithText:@"↓"
                      domCode:ui::DomCode::ARROW_DOWN
                     modifier:modifierNone],
      [self keyButtonWithText:@"←"
                      domCode:ui::DomCode::ARROW_LEFT
                     modifier:modifierNone],
      [self keyButtonWithText:@"→"
                      domCode:ui::DomCode::ARROW_RIGHT
                     modifier:modifierNone],
    ]];
  }
  return self;
}

- (CGSize)intrinsicContentSize {
  return CGSizeMake(UIViewNoIntrinsicMetric, kViewHeight);
}

#pragma mark - Private

- (void)initScrollViewWithButtons:(NSArray<AdvancedKeyButton*>*)buttons {
  _scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];
  _scrollView.translatesAutoresizingMaskIntoConstraints = NO;
  _scrollView.showsVerticalScrollIndicator = NO;
  [self addSubview:_scrollView];

  [NSLayoutConstraint activateConstraints:@[
    [_scrollView.topAnchor constraintEqualToAnchor:self.topAnchor],
    [_scrollView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    [_scrollView.leftAnchor constraintEqualToAnchor:self.leftAnchor],
    [_scrollView.rightAnchor constraintEqualToAnchor:self.rightAnchor],
  ]];

  _buttons = buttons;
  AdvancedKeyButton* lastButton = nil;
  CGFloat contentWidth = 0.f;
  for (AdvancedKeyButton* button in _buttons) {
    [_scrollView addSubview:button];
    if (lastButton) {
      // Using left and right since RTL language doesn't flip the keyboard.
      [button.leftAnchor constraintEqualToAnchor:lastButton.rightAnchor]
          .active = YES;
    }
    // Do not anchor on _scrollView. This will make it not scrollable.
    [NSLayoutConstraint activateConstraints:@[
      [button.topAnchor constraintEqualToAnchor:self.topAnchor],
      [button.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    ]];
    [button sizeToFit];
    contentWidth += button.frame.size.width;
    lastButton = button;
  }

  // contentHeight seems slightly bigger than kViewHeight for some reason.
  CGFloat contentHeight = lastButton.frame.size.height;
  _scrollView.contentSize = CGSizeMake(contentWidth, contentHeight);
}

- (void)keyButtonDidTap:(AdvancedKeyButton*)button {
  if (button.modifier) {
    button.selected = !button.selected;
  }
}

- (AdvancedKeyButton*)keyButtonWithText:(NSString*)text
                                domCode:(ui::DomCode)domCode
                               modifier:(ModifierFlag)modifier {
  AdvancedKeyButton* button = [[AdvancedKeyButton alloc] init];
  button.translatesAutoresizingMaskIntoConstraints = NO;
  button.keycode = static_cast<uint32_t>(domCode);
  button.modifier = modifier;
  button.uppercaseTitle = NO;
  [button setTitle:text forState:UIControlStateNormal];
  [button setTitleColor:RemotingTheme.flatButtonTextColor
               forState:UIControlStateNormal];
  [button setBackgroundColor:RemotingTheme.flatButtonSelectedColor
                    forState:UIControlStateSelected];
  [button addTarget:self
                action:@selector(keyButtonDidTap:)
      forControlEvents:UIControlEventTouchUpInside];
  return button;
}

@end
