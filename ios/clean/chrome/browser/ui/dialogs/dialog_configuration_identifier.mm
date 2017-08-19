// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/dialogs/dialog_configuration_identifier.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Returns a unique ID.
NSUInteger GetUniqueID() {
  static NSUInteger unique_id = 0;
  return ++unique_id;
}
}

@interface DialogConfigurationIdentifier () {
  // the unique identifier for this object.
  NSUInteger _uniqueID;
}

// Designated initializer that takes a unique ID.
- (instancetype)initWithUniqueID:(NSUInteger)uniqueID;

@end

@implementation DialogConfigurationIdentifier

- (instancetype)init {
  return [self initWithUniqueID:GetUniqueID()];
}

- (instancetype)initWithUniqueID:(NSUInteger)uniqueID {
  if ((self = [super init])) {
    _uniqueID = uniqueID;
  }
  return self;
}

#pragma mark - NSObject

- (BOOL)isEqual:(id)object {
  if (!object || ![object isKindOfClass:[self class]])
    return NO;
  DialogConfigurationIdentifier* identifier =
      static_cast<DialogConfigurationIdentifier*>(object);
  return identifier->_uniqueID == _uniqueID;
}

- (NSUInteger)hash {
  return _uniqueID;
}

#pragma mark - NSCopying

- (id)copyWithZone:(NSZone*)zone {
  return [[[self class] allocWithZone:zone] initWithUniqueID:_uniqueID];
}

@end
