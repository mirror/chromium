// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"

#import <Foundation/Foundation.h>

#include "base/mac/bind_objc_block.h"
#import "ios/chrome/browser/snapshots/snapshot_cache.h"
#import "ios/chrome/browser/snapshots/snapshot_cache_factory.h"
#import "ios/chrome/browser/snapshots/snapshot_constants.h"
#import "ios/chrome/browser/web/tab_id_tab_helper.h"
#import "ios/web/public/web_state/navigation_context.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(SnapshotTabHelper);

SnapshotTabHelper::SnapshotTabHelper(web::WebState* web_state)
    : web::WebStateObserver(web_state), web_state_(web_state) {}

SnapshotTabHelper::~SnapshotTabHelper() = default;

void SnapshotTabHelper::CreateForWebState(web::WebState* web_state) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(),
                           base::WrapUnique(new SnapshotTabHelper(web_state)));
  }
}

void SnapshotTabHelper::TakeSnapshot() {
  NSString* tab_id = TabIdTabHelper::FromWebState(web_state_)->tab_id();
  SnapshotCache* snapshot_cache =
      SnapshotCacheFactory::GetForBrowserState(web_state_->GetBrowserState());
  if (!provider_callback_.is_null()) {
    provider_callback_.Run(base::BindBlockArc(^(const gfx::Image& snapshot) {
      [snapshot_cache setImage:snapshot.ToUIImage() withSessionID:tab_id];
    }));
  } else {
    web_state_->TakeSnapshot(base::BindBlockArc(^(const gfx::Image& snapshot) {
                               [snapshot_cache setImage:snapshot.ToUIImage()
                                          withSessionID:tab_id];
                             }),
                             kSnapshotThumbnailSize);
  }
}

void SnapshotTabHelper::SetSnapshotProvider(
    const SnapshotProviderCallback& callback) {
  provider_callback_ = callback;
}

void SnapshotTabHelper::DidFinishNavigation(
    web::NavigationContext* navigation_context) {
  TakeSnapshot();
}

void SnapshotTabHelper::WebStateDestroyed() {
  NSString* tab_id = TabIdTabHelper::FromWebState(web_state_)->tab_id();
  SnapshotCache* snapshot_cache =
      SnapshotCacheFactory::GetForBrowserState(web_state_->GetBrowserState());
  [snapshot_cache removeImageWithSessionID:tab_id];
}
