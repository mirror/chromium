// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_path_cache.h"

#include "base/logging.h"

namespace {
// The current version of the cached position. This number should be incremented
// each time the NSCoding implementation changes.
const int kVersion = 1;

NSString* kBookmarkPathKey = @"BookmarkPathKey";
NSString* kPositionKey = @"PositionKey";
NSString* kVersionKey = @"VersionKey";
}  // namespace

@interface BookmarkPathCache ()

// This is the designated initializer. It does not perform any validation.
- (instancetype)initWithBookmarkPath:(NSArray*)bookmarkPath
                            position:(CGFloat)position
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

@implementation BookmarkPathCache
@synthesize position = _position;
@synthesize bookmarkPath = _bookmarkPath;

#pragma mark - Public Constructors

+ (BookmarkPathCache*)cacheForBookmarkPath:(NSArray*)bookmarkPath
                                  position:(CGFloat)position {
  return [[BookmarkPathCache alloc] initWithBookmarkPath:bookmarkPath
                                                position:position];
}

#pragma mark - Designated Initializer

- (instancetype)initWithBookmarkPath:(NSArray*)bookmarkPath
                            position:(CGFloat)position {
  self = [super init];
  if (self) {
    _bookmarkPath = bookmarkPath;
    _position = position;
  }
  return self;
}

#pragma mark - Superclass Overrides

- (instancetype)init {
  NOTREACHED();
  return nil;
}

- (BOOL)isEqual:(id)object {
  if (self == object)
    return YES;
  if (![object isKindOfClass:[BookmarkPathCache class]])
    return NO;
  BookmarkPathCache* other = static_cast<BookmarkPathCache*>(object);
  if (fabs(self.position - other.position) > 0.01)
    return NO;
  if (![self.bookmarkPath isEqualToArray:other.bookmarkPath])
    return NO;

  return YES;
}

- (NSUInteger)hash {
  return [self.bookmarkPath hash] ^ static_cast<NSUInteger>(self.position);
}

#pragma mark - NSCoding

- (instancetype)initWithCoder:(NSCoder*)coder {
  int version = [coder decodeIntForKey:kVersionKey];
  if (version != kVersion) {
    return nil;
  }

  return [self initWithBookmarkPath:[coder decodeObjectForKey:kBookmarkPathKey]
                           position:[coder decodeFloatForKey:kPositionKey]];
}

- (void)encodeWithCoder:(NSCoder*)coder {
  [coder encodeInt:kVersion forKey:kVersionKey];
  [coder encodeFloat:self.position forKey:kPositionKey];
  [coder encodeObject:self.bookmarkPath forKey:kBookmarkPathKey];
}

@end
