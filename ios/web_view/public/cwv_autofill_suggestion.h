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
// In the case that this suggestion is created from a credit card or address
// profile, filling using a suggestion may fill more than one field at once.
@interface CWVAutofillSuggestion : NSObject

- (instancetype)init NS_UNAVAILABLE;

// The 'name' attribute of the html form element.
@property(nonatomic, copy, readonly) NSString* formName;

// The 'name' attribute of the html field element.
@property(nonatomic, copy, readonly) NSString* fieldName;

// The string that will replace the field with |fieldName|'s value attribute.
@property(nonatomic, copy, readonly) NSString* value;

// If this suggestion is created from a credit card or address profile. This
// property will contain extra information from that profile to help
// differentiate from other suggestions.
@property(nonatomic, copy, readonly, nullable) NSString* displayDescription;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_SUGGESTION_H
