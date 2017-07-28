// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/external_keyboard_detector.h"

// A view to do measurement to figure out whether an external keyboard is
// presented.
//
// The logic is hacky since iOS doesn't provide any API that tells you whether
// an external keyboard is being used. We infer that in these steps:
//
//   1. Add a hidden text field (this view) to the view to be detected and
//      register the keyboardWillShow notification.
//   2. Make the text field first responder.
//   3. keyboardWillShow will get called. The keyboard's end frame will go
//      offscreen if the external keyboard is presented.
//   4. Pass that information to the callback and remove the hidden text field.
//
// Unfortunately there is no easy way to know immediately when the user connects
// or disconnects the keyboard.
@interface ExternalKeyboardDetectorView : UITextField {
  void (^_callback)(bool);
}

- (void)detectOnView:(UIView*)view callback:(void (^)(bool))callback;

@end

@implementation ExternalKeyboardDetectorView

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    self.hidden = YES;

    // This is to force keyboardWillShow to be called even if the soft keyboard
    // will not be shown.
    self.inputAccessoryView = [[UIView alloc] initWithFrame:CGRectZero];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)detectOnView:(UIView*)view callback:(void (^)(bool))callback {
  _callback = callback;
  [view addSubview:self];
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(keyboardWillShow:)
             name:UIKeyboardWillShowNotification
           object:nil];
  [self becomeFirstResponder];
}

#pragma mark - Private

- (void)keyboardWillShow:(NSNotification*)notification {
  CGRect keyboardFrame = [(NSValue*)[notification.userInfo
      objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];

  // iPad will still show the toolbar at the top of the soft keyboard even if
  // the external keyboard is presented, so the safer check is to see if the
  // bottom of the keyboard goes under the screen.
  int keyboardBottom = keyboardFrame.origin.y + keyboardFrame.size.height;
  BOOL isKeyboardOffScreen =
      keyboardBottom > UIScreen.mainScreen.bounds.size.height;
  [self resignFirstResponder];
  [self removeFromSuperview];
  _callback(isKeyboardOffScreen);
}

@end

#pragma mark - ExternalKeyboardDetector

@implementation ExternalKeyboardDetector

+ (void)detectOnView:(UIView*)view callback:(void (^)(bool))callback {
  ExternalKeyboardDetectorView* detectorView =
      [[ExternalKeyboardDetectorView alloc] initWithFrame:CGRectZero];
  [detectorView detectOnView:view callback:callback];
}

@end
