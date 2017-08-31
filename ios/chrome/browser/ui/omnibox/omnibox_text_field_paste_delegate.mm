// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/omnibox_text_field_paste_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OmniboxTextFieldPasteDelegate

// Since UITextPasteConfigurationSupporting is only defined on iOS 11, compiler
// wants these guards.
#ifdef __IPHONE_11_0
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpartial-availability"

- (NSAttributedString*)
textPasteConfigurationSupporting:
    (id<UITextPasteConfigurationSupporting>)textPasteConfigurationSupporting
    combineItemAttributedStrings:(NSArray<NSAttributedString*>*)itemStrings
                        forRange:(UITextRange*)textRange {
  // Return only one item string to avoid repetition, for example when there are
  // both a URL and a string in the pasteboard.
  return itemStrings[0];
}

#pragma clang diagnostic pop
#endif  // __IPHONE_11_0

@end
