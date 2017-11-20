// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_IOS_BROWSER_FAKE_AUTOFILL_AGENT_H_
#define COMPONENTS_AUTOFILL_IOS_BROWSER_FAKE_AUTOFILL_AGENT_H_

#import <Foundation/Foundation.h>

#import "components/autofill/ios/browser/autofill_agent.h"

NS_ASSUME_NONNULL_BEGIN

@class FormSuggestion;

// Autofill agent used in tests. Use to stub out suggestion fetching by
// providing them using in-memory data.
@interface FakeAutofillAgent : AutofillAgent

// Adds a |suggestion| to be returned for a particular |formName|.
- (void)addSuggestion:(FormSuggestion*)suggestion
          forFormName:(NSString*)formName;

// Returns the last selected |suggestion| for |formName|.
- (FormSuggestion*)selectedSuggestionForFormName:(NSString*)formName;

@end

NS_ASSUME_NONNULL_END

#endif  // COMPONENTS_AUTOFILL_IOS_BROWSER_FAKE_AUTOFILL_AGENT_H_
