// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_CONSUMER_H_

#import "ios/chrome/browser/ui/omnibox/autocomplete_suggestion.h"

// Delegate for AutocompleteResultConsumer.
@protocol AutocompleteResultConsumerDelegate<NSObject>

// Called when a row containing a suggestion is clicked.
- (void)didSelectRow:(NSUInteger)row;
// Called when a suggestion in|row| was chosen for appending to omnibox.
- (void)didSelectRowForAppending:(NSUInteger)row;
// Called when a suggestion in |row| was removed.
- (void)didSelectRowForDeletion:(NSUInteger)row;
// Called on scroll.
- (void)didScroll;

@end

// An abstract consumer of autocomplete results.
@protocol AutocompleteResultConsumer<NSObject>
// Updates the current data and forces a redraw. If animation is YES, adds
// CALayer animations to fade the OmniboxPopupMaterialRows in.
- (void)updateMatches:(NSArray<id<AutocompleteSuggestion>>*)result
        withAnimation:(BOOL)animation;
@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_CONSUMER_H_
