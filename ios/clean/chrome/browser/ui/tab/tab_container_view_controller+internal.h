// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TAB_TAB_CONTAINER_VIEW_CONTROLLER_INTERNAL_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TAB_TAB_CONTAINER_VIEW_CONTROLLER_INTERNAL_H_

#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller.h"

#import <UIKit/UIKit.h>

#import "ios/clean/chrome/browser/ui/ui_types.h"

// Internal API for subclasses and categories of TabContainerViewController.
@interface TabContainerViewController (Internal)

// Container views for child view controllers. The child view controller's
// view is added as a subview that fills its container view via autoresizing.
@property(nonatomic, readonly) UIView* findBarView;
@property(nonatomic, readonly) UIView* toolbarView;
@property(nonatomic, readonly) UIView* contentView;

// Status Bar background view. Its size is directly linked to the difference
// between this VC's view topLayoutGuide top anchor and bottom anchor. This
// means that this view will not be displayed on landscape.
@property(nonatomic, readonly) UIView* statusBarBackgroundView;

// Height constraints for toolbarView. This constraint should be included in
// subclass overrides of -subviewConstraints.
@property(nonatomic, readonly) NSLayoutConstraint* toolbarHeightConstraint;

// Helper method for attaching a child view controller.
- (void)attachChildViewController:(UIViewController*)viewController
                        toSubview:(UIView*)subview;

// Helper method for detaching a child view controller.
- (void)detachChildViewController:(UIViewController*)viewController;

// Configures all the subviews. Subclasses may override this method to configure
// additional subviews. Subclass overrides must first call the super method.
- (void)configureSubviews;

// Returns all the constraints required for this view controller. Subclasses
// must override this method.
- (Constraints*)subviewConstraints;
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TAB_TAB_CONTAINER_VIEW_CONTROLLER_INTERNAL_H_
