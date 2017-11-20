// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/autofill/ios/browser/fake_autofill_agent.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FakeAutofillAgent {
  NSMutableDictionary<NSString*, NSMutableArray<FormSuggestion*>*>*
      _suggestionsByFormName;
  NSMutableDictionary<NSString*, FormSuggestion*>*
      _selectedSuggestionByFormName;
}

- (instancetype)initWithPrefService:(PrefService*)prefService
                           webState:(web::WebState*)webState {
  self = [super initWithPrefService:prefService webState:webState];
  if (self) {
    _suggestionsByFormName = [NSMutableDictionary dictionary];
    _selectedSuggestionByFormName = [NSMutableDictionary dictionary];
  }
  return self;
}

#pragma mark - Public Methods

- (void)addSuggestion:(FormSuggestion*)suggestion
          forFormName:(NSString*)formName {
  NSMutableArray* suggestions = _suggestionsByFormName[formName];
  if (!suggestions) {
    suggestions = [NSMutableArray array];
    _suggestionsByFormName[formName] = suggestions;
  }
  [suggestions addObject:suggestion];
}

- (FormSuggestion*)selectedSuggestionForFormName:(NSString*)formName {
  return _selectedSuggestionByFormName[formName];
}

#pragma mark - FormSuggestionProvider

- (void)checkIfSuggestionsAvailableForForm:(NSString*)formName
                                     field:(NSString*)fieldName
                                 fieldType:(NSString*)fieldType
                                      type:(NSString*)type
                                typedValue:(NSString*)typedValue
                                  webState:(web::WebState*)webState
                         completionHandler:
                             (SuggestionsAvailableCompletion)completion {
  completion([_suggestionsByFormName[formName] count] ? YES : NO);
}

- (void)retrieveSuggestionsForForm:(NSString*)formName
                             field:(NSString*)fieldName
                         fieldType:(NSString*)fieldType
                              type:(NSString*)type
                        typedValue:(NSString*)typedValue
                          webState:(web::WebState*)webState
                 completionHandler:(SuggestionsReadyCompletion)completion {
  completion(_suggestionsByFormName[formName], self);
}

- (void)didSelectSuggestion:(FormSuggestion*)suggestion
                   forField:(NSString*)fieldName
                       form:(NSString*)formName
          completionHandler:(SuggestionHandledCompletion)completion {
  _selectedSuggestionByFormName[formName] = suggestion;
  completion();
}

@end
