// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/shell/shell_autofill_delegate.h"

#import <UIKit/UIKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ShellAutofillDelegate ()
// Alert controller to present autofill suggestions.
@property(nonatomic, strong) UIAlertController* alertController;
@end

@implementation ShellAutofillDelegate

@synthesize alertController = _alertController;

- (void)dealloc {
  [_alertController dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - CWVAutofillControllerDelegate methods

- (void)autofillController:(CWVAutofillController*)autofillController
        didFocusOnFormName:(NSString*)formName
                 fieldName:(NSString*)fieldName
                typedValue:(NSString*)typedValue {
  __weak ShellAutofillDelegate* weakSelf = self;
  id completionBlock = ^(NSArray<CWVAutofillFormSuggestion*>* formSuggestions) {
    ShellAutofillDelegate* strongSelf = weakSelf;
    if (!formSuggestions.count || !strongSelf) {
      return;
    }
    UIAlertController* alertController = [UIAlertController
        alertControllerWithTitle:@"Pick a suggestion"
                         message:nil
                  preferredStyle:UIAlertControllerStyleActionSheet];
    [alertController
        addAction:[UIAlertAction actionWithTitle:@"Cancel"
                                           style:UIAlertActionStyleCancel
                                         handler:nil]];

    for (CWVAutofillFormSuggestion* formSuggestion in formSuggestions) {
      NSString* title =
          [NSString stringWithFormat:@"%@ %@", formSuggestion.value,
                                     formSuggestion.displayDescription];
      UIAlertAction* action = [UIAlertAction
          actionWithTitle:title
                    style:UIAlertActionStyleDefault
                  handler:^(UIAlertAction* _Nonnull action) {
                    [autofillController fillFormSuggestion:formSuggestion
                                                  formName:formName
                                                 fieldName:fieldName];
                  }];
      [alertController addAction:action];
    }

    UIAlertAction* clearAction =
        [UIAlertAction actionWithTitle:@"Clear"
                                 style:UIAlertActionStyleDefault
                               handler:^(UIAlertAction* _Nonnull action) {
                                 [autofillController clearFormNamed:formName];
                               }];
    [alertController addAction:clearAction];

    [[UIApplication sharedApplication].keyWindow.rootViewController
        presentViewController:alertController
                     animated:YES
                   completion:nil];

    strongSelf.alertController = alertController;
  };
  [autofillController fetchSuggestionsForFormName:formName
                                        fieldName:fieldName
                                       typedValue:nil
                                  completionBlock:completionBlock];
}

- (void)autofillController:(CWVAutofillController*)autofillController
        didInputInFormName:(NSString*)formName
                 fieldName:(NSString*)fieldName
                typedValue:(NSString*)typedValue {
}

- (void)autofillController:(CWVAutofillController*)autofillController
         didBlurOnFormName:(NSString*)formName
                 fieldName:(NSString*)fieldName
                typedValue:(NSString*)typedValue {
  [_alertController dismissViewControllerAnimated:YES completion:nil];
  _alertController = nil;
}

@end
