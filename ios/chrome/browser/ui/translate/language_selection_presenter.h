// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_PRESENTER_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_PRESENTER_H_

#import <UIKit/UIKit.h>

@protocol LanguageSelectionPresenterDelegate

- (void)languageSelectionPresenterDidDismiss;

@end

// Helper object that manages the positioning and presentation of a language
// selection view controller.
@interface LanguageSelectionPresenter : NSObject

@property(nonatomic, weak) UIViewController* baseViewController;
@property(nonatomic, weak) UIViewController* presentedViewController;
@property(nonatomic, weak) id<LanguageSelectionPresenterDelegate> delegate;

- (void)prepareForPresentation;

- (void)present;

- (void)dismiss;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_PRESENTER_H_
