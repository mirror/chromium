// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@class SnapshotCache;
@protocol SnapshotProvider;

// TabHelper that exposes a |TakeSnapshot| method, which takes a snapshot of a
// web state and puts it in the snapshot cache. A snapshot provider may be set
// to override the snapshot taken. The snapshot is also removed from the cache
// when the web state is destroyed.
class SnapshotTabHelper : public web::WebStateUserData<SnapshotTabHelper>,
                          public web::WebStateObserver {
 public:
  static void CreateForWebState(web::WebState* web_state,
                                SnapshotCache* snapshot_cache);
  ~SnapshotTabHelper() override;

  // Takes the snapshot. A snapshot provider must be set if the last committed
  // URL is a Chrome scheme.
  void TakeSnapshot();

  // Sets a provider that overrides the default web state snapshot.
  void SetSnapshotProvider(id<SnapshotProvider> snapshot_provider);

 private:
  SnapshotTabHelper(web::WebState* web_state, SnapshotCache* snapshot_cache);

  // WebStateObserver
  void WebStateDestroyed() override;

  web::WebState* web_state_;
  SnapshotCache* snapshot_cache_;
  id<SnapshotProvider> snapshot_provider_;

  DISALLOW_COPY_AND_ASSIGN(SnapshotTabHelper);
};

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_
