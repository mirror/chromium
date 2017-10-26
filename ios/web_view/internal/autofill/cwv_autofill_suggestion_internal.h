// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_SUGGESTION_INTERNAL_H
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_SUGGESTION_INTERNAL_H

#import "ios/web_view/public/cwv_autofill_suggestion.h"

@class FormSuggestion;

@interface CWVAutofillSuggestion ()

- (instancetype)initWithFormSuggestion:(FormSuggestion*)formSuggestion
                              formName:(NSString*)formName
                             fieldName:(NSString*)fieldName
    NS_DESIGNATED_INITIALIZER;

// The internal autofill form suggestion.
@property(nonatomic, readonly) FormSuggestion* formSuggestion;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_SUGGESTION_INTERNAL_H
