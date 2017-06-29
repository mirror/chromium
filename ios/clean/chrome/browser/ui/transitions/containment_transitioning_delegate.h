// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TRANSITIONS_CONTAINMENT_TRANSITIONING_DELEGATE_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TRANSITIONS_CONTAINMENT_TRANSITIONING_DELEGATE_H_

@protocol ContainmentTransitioningDelegate

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForAddingChildController:(UIViewController*)addedChild
                    removingChildController:(UIViewController*)removedChild
                               toController:(UIViewController*)parent;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TRANSITIONS_CONTAINMENT_TRANSITIONING_DELEGATE_H_
