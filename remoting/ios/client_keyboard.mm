// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/client_keyboard.h"

#import "remoting/ios/app/advanced_keys_view.h"

// TODO(nicholss): Look into inputView - The custom input view to display when
// the receiver becomes the first responder

@interface ClientKeyboard ()<AdvancedKeysViewDelegate> {
  UIView* _inputView;
  AdvancedKeysView* _advancedKeysView;
}
@end

@implementation ClientKeyboard

@synthesize autocapitalizationType = _autocapitalizationType;
@synthesize autocorrectionType = _autocorrectionType;
@synthesize keyboardAppearance = _keyboardAppearance;
@synthesize keyboardType = _keyboardType;
@synthesize spellCheckingType = _spellCheckingType;

@synthesize selectedTextRange = _selectedTextRange;
@synthesize delegate = _delegate;

// TODO(nicholss): For physical keyboard, look at UIKeyCommand
// https://developer.apple.com/reference/uikit/uikeycommand?language=objc

- (instancetype)init {
  self = [super init];
  if (self) {
    _autocapitalizationType = UITextAutocapitalizationTypeNone;
    _autocorrectionType = UITextAutocorrectionTypeNo;
    _autocorrectionType = UITextAutocorrectionTypeNo;
    _keyboardType = UIKeyboardTypeDefault;
    _spellCheckingType = UITextSpellCheckingTypeNo;

    self.showsSoftKeyboard = NO;
  }
  return self;
}

#pragma mark - UIKeyInput

- (void)insertText:(NSString*)text {
  std::vector<uint32_t> modifiers = _advancedKeysView
                                        ? _advancedKeysView.activeModifiers
                                        : std::vector<uint32_t>();
  [_delegate clientKeyboardShouldSend:text modifiers:modifiers];
  if (_advancedKeysView) {
    [_advancedKeysView releaseAllModifiers];
  }
}

- (void)deleteBackward {
  [_delegate clientKeyboardShouldDelete];
}

- (BOOL)hasText {
  return NO;
}

#pragma mark - UIResponder

- (BOOL)canBecomeFirstResponder {
  return YES;
}

- (BOOL)resignFirstResponder {
  if (self.showsSoftKeyboard) {
    // This translates the action of resigning first responder when the keyboard
    // is showing into hiding the soft keyboard while keeping the view first
    // responder. This is to allow the hide keyboard button on the soft keyboard
    // to work properly with ClientKeyboard's soft keyboard logic, which calls
    // resignFirstResponder.
    // This may cause weird behavior if the superview has multiple responders
    // (text views).
    self.showsSoftKeyboard = NO;
    return NO;
  }
  return [super resignFirstResponder];
}

- (UIView*)inputAccessoryView {
  if (!self.showsSoftKeyboard) {
    _advancedKeysView = nil;
    return _advancedKeysView;
  }
  _advancedKeysView = [[AdvancedKeysView alloc] initWithFrame:CGRectZero];
  _advancedKeysView.delegate = self;
  return _advancedKeysView;
}

- (UIView*)inputView {
  return _inputView;
}

#pragma mark - UITextInputTraits

#pragma mark - AdvancedKeysViewDelegate

- (void)onKeyCombination:(const std::vector<uint32_t>&)combination {
  [_delegate clientKeyboardShouldSendKeyCombination:combination];
}

#pragma mark - Properties

- (void)setShowsSoftKeyboard:(BOOL)showsSoftKeyboard {
  if (self.showsSoftKeyboard == showsSoftKeyboard) {
    return;
  }

  // Returning nil for inputView will fallback to the system soft keyboard.
  // Returning an empty view will effectively hide it.
  _inputView =
      showsSoftKeyboard ? nil : [[UIView alloc] initWithFrame:CGRectZero];

  if (self.isFirstResponder) {
    [self reloadInputViews];
  }
}

- (BOOL)showsSoftKeyboard {
  return _inputView == nil;
}

@end
