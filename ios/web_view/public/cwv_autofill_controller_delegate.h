// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#import "cwv_export.h"

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H

NS_ASSUME_NONNULL_BEGIN

@class CWVAutofillController;
@class CWVAutofillFormSuggestion;

// Block used to commit a form suggestion back to CWVAutofillController.
typedef void (^CWVCommitFormSuggestionBlock)(
    CWVAutofillFormSuggestion* formSuggestion);

CWV_EXPORT
@protocol CWVAutofillControllerDelegate<NSObject>

@optional

// When the form is focused, this may be called if there are suggestions.
// |formSuggestions| a list of potential suggestions.
// |commitFormSuggestionBlock| a block used to commit suggestions.
- (void)autofillController:(CWVAutofillController*)autofillController
          showFormSuggestions:
              (NSArray<CWVAutofillFormSuggestion*>*)formSuggestions
    commitFormSuggestionBlock:
        (CWVCommitFormSuggestionBlock)commitFormSuggestionBlock;

// Called to indicate that the form is no longer ineligible for autofilling.
// Clients should use this to hide their autofill related user interfaces.
- (void)autofillControllerShouldHideFormSuggestions:
    (CWVAutofillController*)autofillController;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H
