// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_FORM_SUGGESTION_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_FORM_SUGGESTION_H

#import <Foundation/Foundation.h>

#import "cwv_export.h"

CWV_EXPORT
@interface CWVAutofillFormSuggestion : NSObject

- (instancetype)init NS_UNAVAILABLE;

@property(nonatomic, readonly) NSString* value;
@property(nonatomic, readonly) NSString* displayDescription;
@property(nonatomic, readonly) NSString* icon;
@property(nonatomic, readonly) NSInteger identifier;

@end

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_FORM_SUGGESTION_H
