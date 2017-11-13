// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/translate/language_selection_consumer.h"

// The accessibility identifier of the cancel button on language picker view.
// NOTE: this should not be used on iOS 9 for testing.
extern NSString* const kLanguagePickerCancelButtonId;

// The accessibility identifier of the done button on language picker view.
// NOTE: this should not be used on iOS 9 for testing.
extern NSString* const kLanguagePickerDoneButtonId;

@protocol LanguageSelectionViewControllerDelegate

- (void)languageSelectedAtIndex:(int)index;

- (void)languageSelectionCanceled;

@end

@interface LanguageSelectionViewController
    : UIViewController<LanguageSelectionConsumer>

@property(nonatomic, weak) id<LanguageSelectionViewControllerDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_VIEW_CONTROLLER_H_
