// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_CONSUMER_H_

#import <Foundation/Foundation.h>

@protocol LanguageSelectionProvider;

@protocol LanguageSelectionConsumer

@property(nonatomic) int languageCount;
@property(nonatomic) int initialLanguageIndex;
@property(nonatomic) int disabledLanguageIndex;
@property(nonatomic, weak) id<LanguageSelectionProvider> provider;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_CONSUMER_H_
