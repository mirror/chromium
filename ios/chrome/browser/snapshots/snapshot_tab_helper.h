// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@class SnapshotCache;

// Observes web::WebState activity and takes snapshots as necessary.
class SnapshotTabHelper : public web::WebStateUserData<SnapshotTabHelper>,
                          public web::WebStateObserver {
 public:
  static void CreateForWebState(web::WebState* web_state,
                                NSString* tab_id,
                                SnapshotCache* snapshot_cache);
  ~SnapshotTabHelper() override;

 private:
  SnapshotTabHelper(web::WebState* web_state,
                    NSString* tab_id,
                    SnapshotCache* snapshot_cache);

  // WebStateObserver
  void DidFinishNavigation(web::NavigationContext* navigation_context) override;
  void WebStateDestroyed() override;

  NSString* tab_id_;
  SnapshotCache* snapshot_cache_;

  DISALLOW_COPY_AND_ASSIGN(SnapshotTabHelper);
};

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_
