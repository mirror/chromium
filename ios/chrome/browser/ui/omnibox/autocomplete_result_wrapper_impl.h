// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_WRAPPER_IMPL_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_WRAPPER_IMPL_H_

#import "ios/chrome/browser/ui/omnibox/autocomplete_result_wrapper.h"

struct AutocompleteMatch;
@interface AutocompleteResultWrapperImpl : NSObject<AutocompleteResultWrapper>

// This is a temporary solution for coloring strings.
@property(nonatomic, assign) BOOL incognito;
@property(nonatomic, assign) BOOL isStarredMatch;
- (void)setMatch:(const AutocompleteMatch&)match;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_WRAPPER_IMPL_H_
