// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_OBSERVER_BRIDGE_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_OBSERVER_BRIDGE_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"
#import "ios/chrome/browser/snapshots/snapshot_cache_observer.h"

@class SnapshotCache;

// Protocol that corresponds to SnapshotCacheObserver API. Allows registering
// Objective-C objects to listen to SnapshotCache events.
@protocol SnapshotCacheObserving<NSObject>
@optional
// Invoked after the |snapshot_cache| was updated with a new snapshot for
// |tab_id|.
- (void)snapshotCache:(SnapshotCache*)snapshotCache
    didUpdateSnapshot:(NSString*)tabID;
@end

// Observer that bridges SnapshotCache events to an Objective-C observer that
// implements the SnapshotCacheObserving protocol. The observer is not owned.
class SnapshotCacheObserverBridge : public SnapshotCacheObserver {
 public:
  explicit SnapshotCacheObserverBridge(id<SnapshotCacheObserving> observer);
  ~SnapshotCacheObserverBridge() override;

 private:
  // SnapshotCacheObserver implementation.
  void SnapshotDidUpdate(SnapshotCache* snapshot_cache,
                         NSString* tab_id) override;

  __weak id<SnapshotCacheObserving> observer_ = nil;

  DISALLOW_COPY_AND_ASSIGN(SnapshotCacheObserverBridge);
};

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_OBSERVER_BRIDGE_H_
