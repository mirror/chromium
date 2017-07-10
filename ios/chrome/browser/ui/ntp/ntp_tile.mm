// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/ntp_tile.h"

namespace {
NSString* kTitleKey = @"title";
NSString* kURLKey = @"URL";
NSString* kFaviconPathKey = @"faviconPath";
NSString* kFallbackTextColorKey = @"fallbackTextColor";
NSString* kFallbackBackgroundColorKey = @"fallbackBackgroundColor";
NSString* kFallbackIsDefaultColorKey = @"fallbackIsDefaultColor";
}

@implementation NTPTile

@synthesize title = _title;
@synthesize URL = _URL;
@synthesize faviconPath = _faviconPath;
@synthesize fallbackTextColor = _fallbackTextColor;
@synthesize fallbackBackgroundColor = _fallbackBackgroundColor;
@synthesize fallbackIsDefaultColor = _fallbackIsDefaultColor;

- (instancetype)initWithTitle:(NSString*)title URL:(NSURL*)URL {
  self = [super init];
  if (self) {
    _title = title;
    _URL = URL;
  }
  return self;
}

- (instancetype)initWithTitle:(NSString*)title
                          URL:(NSURL*)URL
                  faviconPath:(NSString*)faviconPath
            fallbackTextColor:(UIColor*)fallbackTextColor
      fallbackBackgroundColor:(UIColor*)fallbackBackgroundColor
       fallbackIsDefaultColor:(BOOL)fallbackIsDefaultColor {
  self = [super init];
  if (self) {
    _title = title;
    _URL = URL;
    _faviconPath = faviconPath;
    _fallbackTextColor = fallbackTextColor;
    _fallbackBackgroundColor = fallbackBackgroundColor;
    _fallbackIsDefaultColor = fallbackIsDefaultColor;
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  return [self initWithTitle:[aDecoder decodeObjectForKey:kTitleKey]
                          URL:[aDecoder decodeObjectForKey:kURLKey]
                  faviconPath:[aDecoder decodeObjectForKey:kFaviconPathKey]
            fallbackTextColor:[aDecoder
                                  decodeObjectForKey:kFallbackTextColorKey]
      fallbackBackgroundColor:
          [aDecoder decodeObjectForKey:kFallbackBackgroundColorKey]
       fallbackIsDefaultColor:
           [aDecoder decodeObjectForKey:kFallbackIsDefaultColorKey]];
}

- (void)encodeWithCoder:(NSCoder*)aCoder {
  [aCoder encodeObject:self.title forKey:kTitleKey];
  [aCoder encodeObject:self.URL forKey:kURLKey];
  [aCoder encodeObject:self.faviconPath forKey:kFaviconPathKey];
  [aCoder encodeObject:self.fallbackTextColor forKey:kFallbackTextColorKey];
  [aCoder encodeObject:self.fallbackBackgroundColor
                forKey:kFallbackBackgroundColorKey];
  [aCoder encodeBool:self.fallbackIsDefaultColor
              forKey:kFallbackIsDefaultColorKey];
}

@end
