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

typedef void (^CWVCommitFormSuggestionBlock)(
    CWVAutofillFormSuggestion* formSuggestion);

CWV_EXPORT
@protocol CWVAutofillControllerDelegate<NSObject>

@optional
- (void)autofillController:(CWVAutofillController*)autofillController
          showFormSuggestions:
              (NSArray<CWVAutofillFormSuggestion*>*)formSuggestions
    commitFormSuggestionBlock:
        (CWVCommitFormSuggestionBlock)commitFormSuggestionBlock;
- (void)autofillControllerShouldHideFormSuggestions:
    (CWVAutofillController*)autofillController;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H
