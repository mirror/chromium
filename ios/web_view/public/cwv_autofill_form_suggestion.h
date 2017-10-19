// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_FORM_SUGGESTION_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_FORM_SUGGESTION_H

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, CWVFormSuggestionType) {
  // Unsupported suggestion. Use of this type of suggestion will be no-op.
  CWVFormSuggestionTypeUnsupported = 0,
  // Used to identify suggestions that are used to fill the form.
  CWVFormSuggestionTypeAutofill,
  // Can be used to clear the form.
  CWVFormSuggestionTypeClear
};

CWV_EXPORT
// Represents a suggestion for a html form.
@interface CWVAutofillFormSuggestion : NSObject

- (instancetype)init NS_UNAVAILABLE;

// The string in the form to show to the user to represent the suggestion.
@property(nonatomic, readonly) NSString* value;

// An optional user-visible description for this suggestion.
@property(nonatomic, readonly, nullable) NSString* displayDescription;

// Indicates the type of suggestion.
@property(nonatomic, readonly) CWVFormSuggestionType type;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_FORM_SUGGESTION_H
