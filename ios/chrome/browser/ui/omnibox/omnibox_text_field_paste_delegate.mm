// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/omnibox_text_field_paste_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OmniboxTextFieldPasteDelegate

- (NSAttributedString*)
textPasteConfigurationSupporting:
    (id<UITextPasteConfigurationSupporting>)textPasteConfigurationSupporting
    combineItemAttributedStrings:(NSArray<NSAttributedString*>*)itemStrings
                        forRange:(UITextRange*)textRange
    API_AVAILABLE(ios(11.0)) {
  // Return only one item string to avoid repetition, for example when there are
  // both a URL and a string in the pasteboard.
  return itemStrings[0];
}

@end
