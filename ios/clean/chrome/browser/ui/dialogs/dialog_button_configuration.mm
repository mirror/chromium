// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/dialogs/dialog_button_configuration.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface DialogButtonConfiguration ()

// Initializer used by the factory method.
- (instancetype)initWithText:(NSString*)text
                       style:(DialogButtonStyle)style
                  identifier:(id)identifier NS_DESIGNATED_INITIALIZER;

@end

@implementation DialogButtonConfiguration

@synthesize text = _text;
@synthesize style = _style;
@synthesize identifier = _identifier;

- (instancetype)initWithText:(NSString*)text
                       style:(DialogButtonStyle)style
                  identifier:(id)identifier {
  DCHECK(text.length);
  DCHECK(identifier);
  if ((self = [super init])) {
    _text = [text copy];
    _style = style;
    _identifier = identifier;
  }
  return self;
}

#pragma mark - Public

+ (instancetype)itemWithText:(NSString*)text
                       style:(DialogButtonStyle)style
                  identifier:(id)identifier {
  return [[[self class] alloc] initWithText:text
                                      style:style
                                 identifier:identifier];
}

@end
