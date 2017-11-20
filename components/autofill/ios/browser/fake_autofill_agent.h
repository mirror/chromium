// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_IOS_BROWSER_FAKE_AUTOFILL_AGENT_H_
#define COMPONENTS_AUTOFILL_IOS_BROWSER_FAKE_AUTOFILL_AGENT_H_

#import "components/autofill/ios/browser/autofill_agent.h"

NS_ASSUME_NONNULL_BEGIN

@class FormSuggestion;

// Autofill agent used in tests.
@interface FakeAutofillAgent : AutofillAgent

// Use to initialize with some suggestions to return.
- (instancetype)initWithPrefService:(PrefService*)prefService
                           webState:(web::WebState*)webState
              suggestionsByFormName:
                  (NSDictionary<NSString*, NSArray<FormSuggestion*>*>*)
                      suggestionsByFormName;

@end

NS_ASSUME_NONNULL_END

#endif  // COMPONENTS_AUTOFILL_IOS_BROWSER_FAKE_AUTOFILL_AGENT_H_
