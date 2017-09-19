// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

namespace gfx {
class Image;
}

// Callback used to return an image.
typedef base::Callback<void(const gfx::Image&)> ImageCallback;

// Callback to provide a callback that returns an image.
typedef base::Callback<void(const ImageCallback&)> SnapshotProviderCallback;

// Observes web::WebState activity and takes snapshots as necessary.
class SnapshotTabHelper : public web::WebStateUserData<SnapshotTabHelper>,
                          public web::WebStateObserver {
 public:
  static void CreateForWebState(web::WebState* web_state);
  ~SnapshotTabHelper() override;

  // Takes the snapshot. The snapshot provider is used if one is set.
  void TakeSnapshot();

  // Sets a provider that overrides the default web state snapshot.
  void SetSnapshotProvider(const SnapshotProviderCallback& callback);

 private:
  explicit SnapshotTabHelper(web::WebState* web_state);

  // WebStateObserver
  void DidFinishNavigation(web::NavigationContext* navigation_context) override;
  void WebStateDestroyed() override;

  web::WebState* web_state_;
  SnapshotProviderCallback provider_callback_;

  DISALLOW_COPY_AND_ASSIGN(SnapshotTabHelper);
};

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_TAB_HELPER_H_
