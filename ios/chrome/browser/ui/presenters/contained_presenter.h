// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PRESENTERS_CONTAINED_PRESENTER_H_
#define IOS_CHROME_BROWSER_UI_PRESENTERS_CONTAINED_PRESENTER_H_

#import <UIKit/UIKit.h>

@protocol ContainedPresenterDelegate;

// Helper object that manages the positioning and presentation of a contained
// (child) view controller. Implementors determine the specifics of the
// positioning and presentation.
@protocol ContainedPresenter

// The view controller which will contain the presented (child) view controller.
@property(nonatomic, weak) UIViewController* baseViewController;
// The view controller to be presented.
@property(nonatomic, weak) UIViewController* presentedViewController;
// The delegate object which will be told about presentation events.
@property(nonatomic, weak) id<ContainedPresenterDelegate> delegate;

// Prepare the view controllers for presentation. The presented view controller
// should become a child of the base view controller, and have its view
// added to the view hierarchy of that view controller.
- (void)prepareForPresentation;

// Present the presented view controller, animating the presentation if
// |animated| is YES. If |animated| is NO, any layout changes should execute
// synchronously. This is a no-op if the presnted view controller is already
// being presented.
- (void)presentAnimated:(BOOL)animated;

// Dismiss the presented view controller, animating the dismissal if
// |animated| is YES. If |animated| is NO, any layout changes should execute
// synchronously. This is a no-op if the presented view controller is already
// dismissed, or hasn't been presented. Once the presentation completes (or
// synchronously if |animated| is NO), |delegate| should have its
// -containedPresenterDidDismiss: method called, and the presented view
// controller should stop being a child of the base view controller.
- (void)dismissAnimated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_PRESENTERS_CONTAINED_PRESENTER_H_
