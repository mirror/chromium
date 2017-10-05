// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_FORM_SUGGESTION_INTERNAL_H
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_FORM_SUGGESTION_INTERNAL_H

#import "ios/web_view/public/cwv_autofill_form_suggestion.h"

@class FormSuggestion;

@interface CWVAutofillFormSuggestion ()

- (instancetype)initWithFormSuggestion:(FormSuggestion*)formSuggestion
                              formName:(NSString*)formName
                             fieldName:(NSString*)fieldName
    NS_DESIGNATED_INITIALIZER;

@property(nonatomic, readonly) FormSuggestion* formSuggestion;
@property(nonatomic, readonly) NSString* formName;
@property(nonatomic, readonly) NSString* fieldName;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_FORM_SUGGESTION_INTERNAL_H
