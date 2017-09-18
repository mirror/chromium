// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"

#import <Foundation/Foundation.h>

#include "base/mac/bind_objc_block.h"
#import "ios/chrome/browser/snapshots/snapshot_cache.h"
#import "ios/chrome/browser/snapshots/snapshot_constants.h"
#import "ios/web/public/web_state/navigation_context.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(SnapshotTabHelper);

SnapshotTabHelper::SnapshotTabHelper(web::WebState* web_state,
                                     NSString* tab_id,
                                     SnapshotCache* snapshot_cache)
    : web::WebStateObserver(web_state),
      tab_id_(tab_id),
      snapshot_cache_(snapshot_cache) {}

SnapshotTabHelper::~SnapshotTabHelper() = default;

void SnapshotTabHelper::CreateForWebState(web::WebState* web_state,
                                          NSString* tab_id,
                                          SnapshotCache* snapshot_cache) {
  DCHECK(web_state);
  DCHECK(tab_id);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(
        UserDataKey(), base::WrapUnique(new SnapshotTabHelper(web_state, tab_id,
                                                              snapshot_cache)));
  }
}

void SnapshotTabHelper::DidFinishNavigation(
    web::NavigationContext* navigation_context) {
  web::WebState* web_state = navigation_context->GetWebState();
  web_state->TakeSnapshot(base::BindBlockArc(^(const gfx::Image& snapshot) {
                            [snapshot_cache_ setImage:snapshot.ToUIImage()
                                        withSessionID:tab_id_];
                          }),
                          kSnapshotThumbnailSize);
}

void SnapshotTabHelper::WebStateDestroyed() {
  [snapshot_cache_ removeImageWithSessionID:tab_id_];
}
