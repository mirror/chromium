// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_OBSERVER_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_OBSERVER_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"

@class SnapshotCache;

// Interface for listening to events occurring to the SnapshotCache.
class SnapshotCacheObserver {
 public:
  SnapshotCacheObserver();
  virtual ~SnapshotCacheObserver();

  // Invoked after the |snapshot_cache| was updated with a new snapshot for
  // |tab_id|.
  virtual void SnapshotDidUpdate(SnapshotCache* snapshot_cache,
                                 NSString* tab_id);

 private:
  DISALLOW_COPY_AND_ASSIGN(SnapshotCacheObserver);
};

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_OBSERVER_H_
