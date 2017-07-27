// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/dialogs/dialog_text_field_configuration.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface DialogTextFieldConfiguration ()

// Initializer used by the factory method.
- (instancetype)initWithDefaultText:(NSString*)defaultText
                    placeholderText:(NSString*)placeholderText
                             secure:(BOOL)secure
                         idenfifier:(id)identifier NS_DESIGNATED_INITIALIZER;

@end

@implementation DialogTextFieldConfiguration

@synthesize defaultText = _defaultText;
@synthesize placeholderText = _placeholderText;
@synthesize secure = _secure;
@synthesize identifier = _identifier;

- (instancetype)initWithDefaultText:(NSString*)defaultText
                    placeholderText:(NSString*)placeholderText
                             secure:(BOOL)secure
                         idenfifier:(id)identifier {
  DCHECK(identifier);
  if ((self = [super init])) {
    _defaultText = [defaultText copy];
    _placeholderText = [placeholderText copy];
    _secure = secure;
    _identifier = identifier;
  }
  return self;
}

#pragma mark - Public

+ (instancetype)itemWithDefaultText:(NSString*)defaultText
                    placeholderText:(NSString*)placeholderText
                             secure:(BOOL)secure
                         identifier:(id)identifier {
  return [[[self class] alloc] initWithDefaultText:defaultText
                                   placeholderText:placeholderText
                                            secure:secure
                                        idenfifier:identifier];
}

@end
