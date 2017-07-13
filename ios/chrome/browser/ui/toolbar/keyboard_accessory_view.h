// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ACCESSORY_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ACCESSORY_VIEW_H_

#import <UIKit/UIKIt.h>

#import "ios/chrome/browser/ui/toolbar/keyboard_accessory_view_protocol.h"

// Accessory View above the keyboard.
// Supports two modes: one where a Voice Search button is shown, and one where
// a list of buttons based on |buttonTitles| is shown.
// The default mode is the Voice Search mode.
@interface KeyboardAccessoryView
    : UIInputView<KeyboardAccessoryViewProtocol, UIInputViewAudioFeedback>

// Designated initializer. |buttonTitles| lists the titles of the buttons shown
// in the KEY_SHORTCUTS mode. Can be nil or empty. |delegate| receives the
// various events triggered in the view. Not retained, and can be nil.
- (instancetype)initWithButtons:(NSArray<NSString*>*)buttonTitles
                       delegate:(id<KeyboardAccessoryViewDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

- (instancetype)initWithFrame:(CGRect)frame
               inputViewStyle:(UIInputViewStyle)inputViewStyle NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ACCESSORY_VIEW_H_
