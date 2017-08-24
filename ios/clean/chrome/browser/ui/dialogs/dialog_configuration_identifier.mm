// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/dialogs/dialog_configuration_identifier.h"

#include "base/atomic_sequence_num.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
AtomicSequenceNumber g_dialog_configuration_identifier;
}  // namespace

@interface DialogConfigurationIdentifier () {
  // The unique identifier for this object.
  int _uniqueID;
}

// Designated initializer that takes a unique ID.
- (instancetype)initWithUniqueID:(int)uniqueID;

@end

@implementation DialogConfigurationIdentifier

- (instancetype)init {
  return [self initWithUniqueID:g_dialog_configuration_identifier.GetNext()];
}

- (instancetype)initWithUniqueID:(int)uniqueID {
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
  return static_cast<NSUInteger>(_uniqueID);
}

#pragma mark - NSCopying

- (id)copyWithZone:(NSZone*)zone {
  return [[[self class] allocWithZone:zone] initWithUniqueID:_uniqueID];
}

@end
