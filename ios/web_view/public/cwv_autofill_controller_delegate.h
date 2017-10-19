// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVAutofillController;
@class CWVAutofillFormSuggestion;

// Block used to commit a form suggestion back to CWVAutofillController.
typedef void (^CWVFillFormSuggestionBlock)(
    CWVAutofillFormSuggestion* formSuggestion);

CWV_EXPORT
@protocol CWVAutofillControllerDelegate<NSObject>

@optional

// Called when the form is focused and there are autofill suggestions. Clients
// may use this to present UI to select the desired suggestion.
// |formSuggestions| a list of potential suggestions.
// |fillFormSuggestionBlock| a block used to commit suggestions. Fills a form
// with a suggestion when called. Will overwrite any previous suggestions.
- (void)autofillController:(CWVAutofillController*)autofillController
        showFormSuggestions:
            (NSArray<CWVAutofillFormSuggestion*>*)formSuggestions
    fillFormSuggestionBlock:(CWVFillFormSuggestionBlock)fillFormSuggestionBlock;

// Called to indicate that the form is no longer eligible for autofilling.
// Clients should use this to hide their autofill related user interfaces.
- (void)autofillControllerShouldHideFormSuggestions:
    (CWVAutofillController*)autofillController;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H
