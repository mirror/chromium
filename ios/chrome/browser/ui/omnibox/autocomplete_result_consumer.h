// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_CONSUMER_H_

#import "ios/chrome/browser/ui/omnibox/autocomplete_result_wrapper.h"

@protocol AutocompleteResultConsumerDelegate<NSObject>

- (void)didSelectRow:(NSUInteger)row;
- (void)didSelectRowForAppending:(NSUInteger)row;
- (void)didSelectRowForDeletion:(NSUInteger)row;
- (void)didScroll;

@end

@protocol AutocompleteResultConsumer<NSObject>
// Updates the current data and forces a redraw. If animation is YES, adds
// CALayer animations to fade the OmniboxPopupMaterialRows in.
- (void)updateMatches:(NSArray<id<AutocompleteResultWrapper>>*)result
        withAnimation:(BOOL)animation;
@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_CONSUMER_H_
