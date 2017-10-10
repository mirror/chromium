// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/favicon/favicon_attributes.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FaviconAttributes
@synthesize faviconImage = _faviconImage;
@synthesize monogramString = _monogramString;
@synthesize textColor = _textColor;
@synthesize backgroundColor = _backgroundColor;
@synthesize defaultBackgroundColor = _defaultBackgroundColor;
@synthesize iconType = _iconType;

// Designated initializer. Either |image| or all of |textColor|,
// |backgroundColor| and |monogram| must be not nil.
- (instancetype)initWithImage:(UIImage*)image
                     monogram:(NSString*)monogram
                    textColor:(UIColor*)textColor
              backgroundColor:(UIColor*)backgroundColor
       defaultBackgroundColor:(BOOL)defaultBackgroundColor
                     iconType:(favicon_base::IconType)iconType {
  DCHECK(image || (monogram && textColor && backgroundColor));
  self = [super init];
  if (self) {
    _faviconImage = image;
    _monogramString = [monogram copy];
    _textColor = textColor;
    _backgroundColor = backgroundColor;
    _defaultBackgroundColor = defaultBackgroundColor;
    _iconType = iconType;
  }

  return self;
}

+ (instancetype)attributesWithImage:(UIImage*)image
                           iconType:(favicon_base::IconType)iconType {
  DCHECK(image);
  return [[self alloc] initWithImage:image
                            monogram:nil
                           textColor:nil
                     backgroundColor:nil
              defaultBackgroundColor:NO
                            iconType:iconType];
}

+ (instancetype)attributesWithMonogram:(NSString*)monogram
                             textColor:(UIColor*)textColor
                       backgroundColor:(UIColor*)backgroundColor
                defaultBackgroundColor:(BOOL)defaultBackgroundColor
                              iconType:(favicon_base::IconType)iconType {
  return [[self alloc] initWithImage:nil
                            monogram:monogram
                           textColor:textColor
                     backgroundColor:backgroundColor
              defaultBackgroundColor:defaultBackgroundColor
                            iconType:iconType];
}

+ (ntp_tiles::TileVisualType)tileVisualTypeFromAttributes:
    (nullable FaviconAttributes*)attributes {
  if (!attributes) {
    return ntp_tiles::TileVisualType::NONE;
  } else if (attributes.faviconImage) {
    return ntp_tiles::TileVisualType::ICON_REAL;
  }
  return attributes.defaultBackgroundColor
             ? ntp_tiles::TileVisualType::ICON_DEFAULT
             : ntp_tiles::TileVisualType::ICON_COLOR;
}

@end
