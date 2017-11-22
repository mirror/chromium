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
    didFocusOnFieldWithName:(NSString*)fieldName
                   formName:(NSString*)formName
                      value:(NSString*)value {
  __weak ShellAutofillDelegate* weakSelf = self;
  id completionBlock = ^(NSArray<CWVAutofillSuggestion*>* suggestions) {
    ShellAutofillDelegate* strongSelf = weakSelf;
    if (!suggestions.count || !strongSelf) {
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

    for (CWVAutofillSuggestion* suggestion in suggestions) {
      NSString* title =
          [NSString stringWithFormat:@"%@ %@", suggestion.value,
                                     suggestion.displayDescription];
      UIAlertAction* action =
          [UIAlertAction actionWithTitle:title
                                   style:UIAlertActionStyleDefault
                                 handler:^(UIAlertAction* _Nonnull action) {
                                   [autofillController fillSuggestion:suggestion
                                                    completionHandler:nil];
                                 }];
      [alertController addAction:action];
    }

    UIAlertAction* clearAction =
        [UIAlertAction actionWithTitle:@"Clear"
                                 style:UIAlertActionStyleDefault
                               handler:^(UIAlertAction* _Nonnull action) {
                                 [autofillController clearFormWithName:formName
                                                     completionHandler:nil];
                               }];
    [alertController addAction:clearAction];

    [[UIApplication sharedApplication].keyWindow.rootViewController
        presentViewController:alertController
                     animated:YES
                   completion:nil];

    strongSelf.alertController = alertController;
  };
  [autofillController fetchSuggestionsForFormWithName:formName
                                            fieldName:fieldName
                                    completionHandler:completionBlock];
}

- (void)autofillController:(CWVAutofillController*)autofillController
    didInputInFieldWithName:(NSString*)fieldName
                   formName:(NSString*)formName
                      value:(NSString*)value {
}

- (void)autofillController:(CWVAutofillController*)autofillController
    didBlurOnFieldWithName:(NSString*)fieldName
                  formName:(NSString*)formName
                     value:(NSString*)value {
  [_alertController dismissViewControllerAnimated:YES completion:nil];
  _alertController = nil;
}

@end
