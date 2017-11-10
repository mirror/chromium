// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_CONTEXT_H_
#define IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_CONTEXT_H_

#import <Foundation/Foundation.h>

namespace translate {
class TranslateInfoBarDelegate;
}

@interface LanguageSelectionContext : NSObject

+ (instancetype)contextWithLanguageData:
                    (translate::TranslateInfoBarDelegate*)languageData
                           initialIndex:(size_t)initialLanguageIndex
                       unavailableIndex:(size_t)unavailableLanguageIndex;

@property(nonatomic, readonly)
    const translate::TranslateInfoBarDelegate* languageData;
@property(nonatomic, readonly) size_t initialLanguageIndex;
@property(nonatomic, readonly) size_t unavailableLanguageIndex;

@end

#endif  // IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_CONTEXT_H_
