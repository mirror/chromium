// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/clipping_textfield_container.h"

#include "base/strings/sys_string_conversions.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "ios/chrome/browser/autocomplete/autocomplete_scheme_classifier_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ClippingTextFieldContainer ()

@property(nonatomic, strong) NSLayoutConstraint* leadingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* trailingConstraint;

@end

@implementation ClippingTextFieldContainer
@synthesize textField = _textField;
@synthesize leadingConstraint = _leadingConstraint;
@synthesize trailingConstraint = _trailingConstraint;

- (instancetype)initWithClippingTextField:(ClippingTextField*)textField {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _textField = textField;
    textField.clippingTextfieldDelegate = self;
    [self addSubview:textField];

    // Configure layout.
    _leadingConstraint = [self.textField.leadingAnchor
        constraintEqualToAnchor:self.leadingAnchor];
    _trailingConstraint = [self.textField.trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor];
    [NSLayoutConstraint activateConstraints:@[
      [textField.topAnchor constraintEqualToAnchor:self.topAnchor],
      [textField.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
      _leadingConstraint, _trailingConstraint
    ]];
    textField.translatesAutoresizingMaskIntoConstraints = NO;
    self.clipsToBounds = YES;

    self.backgroundColor = [[UIColor redColor] colorWithAlphaComponent:0.3];
  }
  return self;
}

- (void)startClipping {
  CGFloat suffixWidth = 0;
  CGFloat prefixWidth =
      -[self leadingConstantWithAttributedText:self.textField.attributedText
                                 textAlignment:self.textField.textAlignment
                              trailingConstant:&suffixWidth];

  self.leadingConstraint.constant = prefixWidth;
  self.trailingConstraint.constant = suffixWidth;
}

- (void)stopClipping {
  self.leadingConstraint.constant = 0;
  self.trailingConstraint.constant = 0;
}

#pragma mark calculate clipping

- (CGFloat)leadingConstantWithAttributedText:(NSAttributedString*)attributedText
                               textAlignment:(NSTextAlignment)alignment
                            trailingConstant:(out CGFloat*)trailing {
  CGFloat widthOfClippedPrefix = 0;
  url::Component scheme, host;
  AutocompleteInput::ParseForEmphasizeComponents(
      base::SysNSStringToUTF16(attributedText.string),
      AutocompleteSchemeClassifierImpl(), &scheme, &host);
  if (host.len < 0) {
    return 0;
  }
  NSRange hostRange = NSMakeRange(0, host.begin + host.len);
  NSAttributedString* hostString =
      [attributedText attributedSubstringFromRange:hostRange];
  CGFloat widthOfHost = ceil(hostString.size.width);
  widthOfClippedPrefix = MAX(widthOfHost - self.bounds.size.width, 0);

  // Now determine if there is any text that will need to be truncated because
  // there's not enough room.
  int textWidth = ceil(attributedText.size.width);
  CGFloat widthOfClippedSuffix =
      MAX(textWidth - self.bounds.size.width - widthOfClippedPrefix, 0);
  //  BOOL suffixClipped = widthOfClippedSuffix > 0;

  //  // Fade the beginning and/or end of the visible string to indicate to the
  //  user
  //  // that the URL has been clipped.
  //  BOOL prefixClipped = widthOfClippedPrefix > 0;
  //  if (prefixClipped || suffixClipped) {
  //    UIImage* fade = nil;
  //    if ([self textAlignment] == NSTextAlignmentRight) {
  //      // Swap prefix and suffix for RTL.
  //      fade = [GTMFadeTruncatingLabel getLinearGradient:rect
  //                                              fadeHead:suffixClipped
  //                                              fadeTail:prefixClipped];
  //    } else {
  //      fade = [GTMFadeTruncatingLabel getLinearGradient:rect
  //                                              fadeHead:prefixClipped
  //                                              fadeTail:suffixClipped];
  //    }
  //    CGContextClipToMask(UIGraphicsGetCurrentContext(), rect, fade.CGImage);
  //  }

  *trailing = widthOfClippedSuffix;

  return widthOfClippedPrefix;
}

#pragma mark - ClippingTextFieldDelegate

- (void)textFieldTextChanged:(UITextField*)sender {
  [self startClipping];
}

- (void)textFieldBecameFirstResponder:(UITextField*)sender {
  [self stopClipping];
}

- (void)textFieldResignedFirstResponder:(UITextField*)sender {
  [self startClipping];
}

@end
