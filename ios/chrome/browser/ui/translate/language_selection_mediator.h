// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_MEDIATOR_H_

#import <Foundation/Foundation.h>
#include <string>

@class LanguageSelectionContext;
@protocol LanguageSelectionConsumer;

@interface LanguageSelectionMediator : NSObject

- (instancetype)initWithContext:(LanguageSelectionContext*)context;

@property(nonatomic, weak) id<LanguageSelectionConsumer> consumer;

- (std::string)languageCodeForLanguageAtIndex:(int)index;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_MEDIATOR_H_
