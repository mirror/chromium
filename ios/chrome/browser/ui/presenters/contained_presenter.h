// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PRESENTERS_CONTAINED_PRESENTER_H_
#define IOS_CHROME_BROWSER_UI_PRESENTERS_CONTAINED_PRESENTER_H_

#import <UIKit/UIKit.h>

@protocol ContainedPresenterDelegate;

// Helper object that manages the positioning and presentation of a language
// selection view controller.
@protocol ContainedPresenter

@property(nonatomic, weak) UIViewController* baseViewController;
@property(nonatomic, weak) UIViewController* presentedViewController;
@property(nonatomic, weak) id<ContainedPresenterDelegate> delegate;

- (void)prepareForPresentation;

- (void)presentAnimated:(BOOL)animated;

- (void)dismissAnimated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_PRESENTERS_CONTAINED_PRESENTER_H_
