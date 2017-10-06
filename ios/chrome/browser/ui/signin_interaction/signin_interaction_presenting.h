// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_PRESENTING_H_
#define IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_PRESENTING_H_

#import <UIKit/UIKit.h>

#import "base/ios/block_types.h"

// A protocol for objects that present Sign In UI.
@protocol SigninInteractionPresenting

- (void)presentViewController:(UIViewController*)viewController
                     animated:(BOOL)animated
                   completion:(ProceduralBlock)completion;

- (void)presentTopViewController:(UIViewController*)viewController
                        animated:(BOOL)animated
                      completion:(ProceduralBlock)completion;

- (void)dismissViewControllerAnimated:(BOOL)animated
                           completion:(ProceduralBlock)completion;

- (void)presentError:(NSError*)error
       dismissAction:(ProceduralBlock)dismissAction;

- (void)dismissError;

@property(nonatomic, assign, readonly) BOOL isPresenting;

@end

#endif  // IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_PRESENTING_H_
