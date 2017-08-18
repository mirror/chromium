// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/omnibox_text_field_paste_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface OmniboxTextFieldPasteDelegate ()

@property(nonatomic, strong) NSString* link;

@end

@implementation OmniboxTextFieldPasteDelegate

- (void)textPasteConfigurationSupporting:
            (id<UITextPasteConfigurationSupporting>)
                textPasteConfigurationSupporting
                      transformPasteItem:(id<UITextPasteItem>)item {
}

- (NSAttributedString*)
textPasteConfigurationSupporting:
    (id<UITextPasteConfigurationSupporting>)textPasteConfigurationSupporting
    combineItemAttributedStrings:(NSArray<NSAttributedString*>*)itemStrings
                        forRange:(UITextRange*)textRange {
  if (self.link) {
    NSAttributedString* string =
        [[NSAttributedString alloc] initWithString:link];
    self.link = 0;
    return string;
  }

  return nil;
}

@end
