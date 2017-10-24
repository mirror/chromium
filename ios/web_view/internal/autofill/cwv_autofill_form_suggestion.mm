// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_form_suggestion_internal.h"

#include "components/autofill/core/browser/popup_item_ids.h"
#import "components/autofill/ios/browser/form_suggestion.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CWVAutofillFormSuggestion

@synthesize formSuggestion = _formSuggestion;
@synthesize formName = _formName;
@synthesize fieldName = _fieldName;

- (instancetype)initWithFormSuggestion:(FormSuggestion*)formSuggestion
                              formName:(NSString*)formName
                             fieldName:(NSString*)fieldName {
  self = [super init];
  if (self) {
    _formSuggestion = formSuggestion;
    _formName = formName;
    _fieldName = fieldName;
  }
  return self;
}

#pragma mark - Public Methods

- (NSString*)value {
  return _formSuggestion.value;
}

- (NSString*)displayDescription {
  return _formSuggestion.displayDescription;
}

#pragma mark - NSObject

- (NSString*)debugDescription {
  return [NSString stringWithFormat:@"%@ value: %@, displayDescription: %@",
                                    [super description], self.value,
                                    self.displayDescription];
}

@end
