// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/snapshots/snapshot_cache_observer_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

SnapshotCacheObserverBridge::SnapshotCacheObserverBridge(
    id<SnapshotCacheObserving> observer)
    : observer_(observer) {}

SnapshotCacheObserverBridge::~SnapshotCacheObserverBridge() {}

void SnapshotCacheObserverBridge::SnapshotDidUpdate(
    SnapshotCache* snapshot_cache,
    NSString* tab_id) {
  const SEL selector = @selector(snapshotCache:didUpdateSnapshot:);
  if (![observer_ respondsToSelector:selector])
    return;
  [observer_ snapshotCache:snapshot_cache didUpdateSnapshot:tab_id];
}
