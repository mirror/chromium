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

- (void)
     autofillController:(CWVAutofillController*)autofillController
    showFormSuggestions:(NSArray<CWVAutofillFormSuggestion*>*)formSuggestions
fillFormSuggestionBlock:(CWVFillFormSuggestionBlock)fillFormSuggestionBlock {
  UIAlertController* alertController = [UIAlertController
      alertControllerWithTitle:@"Pick a suggestion"
                       message:nil
                preferredStyle:UIAlertControllerStyleActionSheet];
  [alertController
      addAction:[UIAlertAction actionWithTitle:@"Cancel"
                                         style:UIAlertActionStyleCancel
                                       handler:nil]];

  for (CWVAutofillFormSuggestion* formSuggestion in formSuggestions) {
    NSString* title;
    if (formSuggestion.type == CWVFormSuggestionTypeAutofill) {
      title = [NSString stringWithFormat:@"%@ %@", formSuggestion.value,
                                         formSuggestion.displayDescription];
    } else if (formSuggestion.type == CWVFormSuggestionTypeClear) {
      title = @"Clear form";
    } else {
      continue;
    }
    [alertController
        addAction:[UIAlertAction
                      actionWithTitle:title
                                style:UIAlertActionStyleDefault
                              handler:^(UIAlertAction* _Nonnull action) {
                                fillFormSuggestionBlock(formSuggestion);
                              }]];
  }

  _alertController = alertController;

  [[UIApplication sharedApplication].keyWindow.rootViewController
      presentViewController:_alertController
                   animated:YES
                 completion:nil];
}

- (void)autofillControllerShouldHideFormSuggestions:
    (CWVAutofillController*)autofillController {
  [_alertController dismissViewControllerAnimated:YES completion:nil];
  _alertController = nil;
}

@end
