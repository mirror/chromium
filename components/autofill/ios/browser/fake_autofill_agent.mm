// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/autofill/ios/browser/fake_autofill_agent.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FakeAutofillAgent {
  NSDictionary<NSString*, NSArray<FormSuggestion*>*>* _suggestionsByFormName;
}

- (instancetype)initWithPrefService:(PrefService*)prefService
                           webState:(web::WebState*)webState
              suggestionsByFormName:
                  (NSDictionary<NSString*, NSArray<FormSuggestion*>*>*)
                      suggestionsByFormName {
  self = [super initWithPrefService:prefService webState:webState];
  if (self) {
    _suggestionsByFormName = suggestionsByFormName;
  }
  return self;
}

- (void)checkIfSuggestionsAvailableForForm:(NSString*)formName
                                     field:(NSString*)fieldName
                                 fieldType:(NSString*)fieldType
                                      type:(NSString*)type
                                typedValue:(NSString*)typedValue
                                  webState:(web::WebState*)webState
                         completionHandler:
                             (SuggestionsAvailableCompletion)completion {
  completion([_suggestionsByFormName[formName] count]);
}

- (void)retrieveSuggestionsForForm:(NSString*)formName
                             field:(NSString*)fieldName
                         fieldType:(NSString*)fieldType
                              type:(NSString*)type
                        typedValue:(NSString*)typedValue
                          webState:(web::WebState*)webState
                 completionHandler:(SuggestionsReadyCompletion)completion {
  completion(_suggestionsByFormName[formName], nil);
}

- (void)didSelectSuggestion:(FormSuggestion*)suggestion
                   forField:(NSString*)fieldName
                       form:(NSString*)formName
          completionHandler:(SuggestionHandledCompletion)completion {
  completion();
}

@end
