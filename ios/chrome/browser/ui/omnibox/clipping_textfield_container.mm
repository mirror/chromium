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

@property(nonatomic, strong) NSLayoutConstraint* leftConstraint;
@property(nonatomic, strong) NSLayoutConstraint* rightConstraint;
@property(nonatomic, strong) UIImageView* maskingImageView;
@property(nonatomic, assign, getter=isClipping) BOOL clipping;
@end

@implementation ClippingTextFieldContainer
@synthesize textField = _textField;
@synthesize leftConstraint = _leftConstraint;
@synthesize rightConstraint = _rightConstraint;
@synthesize maskingImageView = _maskingImageView;
@synthesize clipping = _clipping;

- (instancetype)initWithClippingTextField:(ClippingTextField*)textField {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _textField = textField;
    textField.clippingTextfieldDelegate = self;
    [self addSubview:textField];

    _maskingImageView = [[UIImageView alloc] init];
    _maskingImageView.backgroundColor = [UIColor clearColor];
    [self addSubview:_maskingImageView];

    // Configure layout.
    _leftConstraint =
        [self.textField.leftAnchor constraintEqualToAnchor:self.leftAnchor];
    _rightConstraint =
        [self.textField.rightAnchor constraintEqualToAnchor:self.rightAnchor];
    [NSLayoutConstraint activateConstraints:@[
      [textField.topAnchor constraintEqualToAnchor:self.topAnchor],
      [textField.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
      _leftConstraint,
      _rightConstraint,
      [_maskingImageView.topAnchor constraintEqualToAnchor:self.topAnchor],
      [_maskingImageView.bottomAnchor
          constraintEqualToAnchor:self.bottomAnchor],
      [_maskingImageView.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor],
      [_maskingImageView.leadingAnchor
          constraintEqualToAnchor:self.leadingAnchor],

    ]];
    textField.translatesAutoresizingMaskIntoConstraints = NO;
    _maskingImageView.translatesAutoresizingMaskIntoConstraints = NO;
    self.clipsToBounds = YES;

    _clipping = NO;
  }
  return self;
}

- (void)layoutSubviews {
  if ([self isClipping]) {
    [self applyClipping];
  }
  [super layoutSubviews];
}

- (void)startClipping {
  self.clipping = YES;
  [self applyClipping];
  [self setNeedsLayout];
  [self layoutIfNeeded];
}

- (void)applyClipping {
  CGFloat suffixWidth = 0;
  CGFloat prefixWidth =
      -[self leadingConstantWithAttributedText:self.textField.attributedText
                              trailingConstant:&suffixWidth];

  [self applyGradientsToPrefix:(prefixWidth < 0) suffix:(suffixWidth > 0)];

  self.leftConstraint.constant = prefixWidth;
  self.rightConstraint.constant = suffixWidth;
}

- (void)stopClipping {
  if (![self isClipping]) {
    return;
  }
  self.clipping = NO;
  self.leftConstraint.constant = 0;
  self.rightConstraint.constant = 0;
  [self removeGradient];
}

// Fade the beginning and/or end of the visible string to indicate to the user
// that the URL has been clipped.
- (void)applyGradientsToPrefix:(BOOL)shouldApplyToPrefix
                        suffix:(BOOL)shouldApplyToSuffix {
  UIImage* fade = nil;
  if (shouldApplyToPrefix || shouldApplyToSuffix) {
    if (self.textField.textAlignment == NSTextAlignmentRight) {
      // Swap prefix and suffix for RTL.
      fade = [ClippingTextFieldContainer getLinearGradient:self.bounds
                                                  fadeHead:shouldApplyToSuffix
                                                  fadeTail:shouldApplyToPrefix];
    } else {
      fade = [ClippingTextFieldContainer getLinearGradient:self.bounds
                                                  fadeHead:shouldApplyToPrefix
                                                  fadeTail:shouldApplyToSuffix];
    }
  }

  self.maskingImageView.image = fade;
}

- (void)removeGradient {
  [self applyGradientsToPrefix:NO suffix:NO];
}

#pragma mark calculate clipping

// Calculates the length (in pts) of the clipped text on the leading and
// trailing sides of the omnibox in order to show the most significant part of
// the hostname.
- (CGFloat)leadingConstantWithAttributedText:(NSAttributedString*)attributedText
                            trailingConstant:(out CGFloat*)trailing {
  // The goal is to always show the most significant part of the hostname
  // (i.e. the end of the TLD).
  //
  //                     --------------------
  // www.somereallyreally|longdomainname.com|/path/gets/clipped
  //                     --------------------
  // {  clipped prefix  } {  visible text  } { clipped suffix }

  // First find how much (if any) of the scheme/host needs to be clipped so that
  // the end of the TLD fits in bounds.

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

#pragma mark - gradient generator

+ (UIImage*)getLinearGradient:(CGRect)rect
                     fadeHead:(BOOL)fadeHead
                     fadeTail:(BOOL)fadeTail {
  // Create an opaque context.
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
  CGContextRef context = CGBitmapContextCreate(
      NULL, rect.size.width, rect.size.height, 8, 4 * rect.size.width,
      colorSpace, (CGBitmapInfo)kCGImageAlphaOnly);

  // White background will mask opaque, black gradient will mask transparent.
  CGContextSetFillColorWithColor(context, [UIColor clearColor].CGColor);
  CGContextFillRect(context, rect);

  // Create gradient from white to black.
  CGFloat locs[2] = {0.0f, 1.0f};
  CGFloat components[4] = {0.0f, 0.0f, 1.0f, 1.0f};
  CGGradientRef gradient =
      CGGradientCreateWithColorComponents(colorSpace, components, locs, 2);
  CGColorSpaceRelease(colorSpace);

  // Draw head and/or tail gradient.
  CGFloat fadeWidth = MIN(rect.size.height * 2, floor(rect.size.width / 4));
  CGFloat minX = CGRectGetMinX(rect);
  CGFloat maxX = CGRectGetMaxX(rect);
  if (fadeTail) {
    CGFloat startX = maxX - fadeWidth;
    CGPoint startPoint = CGPointMake(startX, CGRectGetMidY(rect));
    CGPoint endPoint = CGPointMake(maxX, CGRectGetMidY(rect));
    CGContextDrawLinearGradient(context, gradient, startPoint, endPoint, 0);
  }
  if (fadeHead) {
    CGFloat startX = minX + fadeWidth;
    CGPoint startPoint = CGPointMake(startX, CGRectGetMidY(rect));
    CGPoint endPoint = CGPointMake(minX, CGRectGetMidY(rect));
    CGContextDrawLinearGradient(context, gradient, startPoint, endPoint, 0);
  }
  CGGradientRelease(gradient);

  // Clean up, return image.
  CGImageRef ref = CGBitmapContextCreateImage(context);
  UIImage* image = [UIImage imageWithCGImage:ref];
  CGImageRelease(ref);
  CGContextRelease(context);
  return image;
}

@end
