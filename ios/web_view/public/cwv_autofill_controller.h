// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_H

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVAutofillFormSuggestion;
@protocol CWVAutofillControllerDelegate;

CWV_EXPORT
// Exposes autofill features.
@interface CWVAutofillController : NSObject

// Delegate to receive autofill callbacks.
@property(nonatomic, weak, nullable) id<CWVAutofillControllerDelegate> delegate;

- (void)clearFormNamed:(NSString*)formName;

- (void)fetchSuggestionsForFormName:(NSString*)formName
                          fieldName:(NSString*)fieldName
                         typedValue:(nullable NSString*)typedValue
                    completionBlock:
                        (void (^)(NSArray<CWVAutofillFormSuggestion*>*))
                            completionBlock;

- (void)fillFormSuggestion:(CWVAutofillFormSuggestion*)formSuggestion
                  formName:(NSString*)formName
                 fieldName:(NSString*)fieldName;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_H
