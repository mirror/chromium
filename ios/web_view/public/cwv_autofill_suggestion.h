// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_SUGGESTION_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_SUGGESTION_H

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
// Represents a suggestion for a form based off of a single field.
// Note that filling using a suggestion may fill more than one field at once.
@interface CWVAutofillSuggestion : NSObject

- (instancetype)init NS_UNAVAILABLE;

// The 'name' attribute of the html form.
@property(nonatomic, readonly) NSString* formName;

// the 'name' attribute of the html field.
@property(nonatomic, readonly) NSString* fieldName;

// The string that will replace the field with |fieldName|'s value attribute.
@property(nonatomic, readonly) NSString* value;

// An optional user-visible description for this suggestion.
@property(nonatomic, readonly, nullable) NSString* displayDescription;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_SUGGESTION_H
