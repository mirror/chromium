// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_EXTERNAL_KEYBOARD_DETECTOR_H_
#define REMOTING_IOS_EXTERNAL_KEYBOARD_DETECTOR_H_

#import <UIKit/UIKit.h>

// A class for detecting whether an external keyboard is presented.
@interface ExternalKeyboardDetector : NSObject

// |callback| will be called with YES if an external keyboard is presented.
+ (void)detectOnView:(UIView*)view callback:(void (^)(bool))callback;

@end

#endif  // REMOTING_IOS_EXTERNAL_KEYBOARD_DETECTOR_H_
