// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_list/web_state_list_metrics_observer.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "ios/chrome/browser/metrics/tab_usage_recorder.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

WebStateListMetricsObserver::WebStateListMetricsObserver(
    TabUsageRecorder* tab_usage_recorder)
    : tab_usage_recorder_(tab_usage_recorder) {
  ResetSessionMetrics();
}

WebStateListMetricsObserver::~WebStateListMetricsObserver() = default;

void WebStateListMetricsObserver::ResetSessionMetrics() {
  inserted_web_state_counter_ = 0;
  detached_web_state_counter_ = 0;
  activated_web_state_counter_ = 0;
}

void WebStateListMetricsObserver::RecordSessionMetrics() {
  UMA_HISTOGRAM_CUSTOM_COUNTS("Session.ClosedTabCounts",
                              detached_web_state_counter_, 1, 200, 50);
  UMA_HISTOGRAM_CUSTOM_COUNTS("Session.OpenedTabCounts",
                              activated_web_state_counter_, 1, 200, 50);
  UMA_HISTOGRAM_CUSTOM_COUNTS("Session.NewTabCounts",
                              inserted_web_state_counter_, 1, 200, 50);
}

void WebStateListMetricsObserver::WebStateInsertedAt(
    WebStateList* web_state_list,
    web::WebState* web_state,
    int index,
    bool foreground) {
  base::RecordAction(base::UserMetricsAction("MobileNewTabOpened"));
  ++inserted_web_state_counter_;

  if (tab_usage_recorder_ && foreground) {
    Tab* tab = LegacyTabHelper::GetTabForWebState(web_state);
    tab_usage_recorder_->TabCreatedForSelection(tab);
  }
}

void WebStateListMetricsObserver::WebStateReplacedAt(
    WebStateList* web_state_list,
    web::WebState* old_web_state,
    web::WebState* new_web_state,
    int index) {
  // Record a tab clobber, since swapping tabs bypasses the Tab code that would
  // normally log clobbers.
  base::RecordAction(base::UserMetricsAction("MobileTabClobbered"));
}

void WebStateListMetricsObserver::WebStateDetachedAt(
    WebStateList* web_state_list,
    web::WebState* web_state,
    int index) {
  base::RecordAction(base::UserMetricsAction("MobileTabClosed"));
  ++detached_web_state_counter_;
}

void WebStateListMetricsObserver::WebStateActivatedAt(
    WebStateList* web_state_list,
    web::WebState* old_web_state,
    web::WebState* new_web_state,
    int active_index,
    bool user_action) {
  ++activated_web_state_counter_;
  if (!user_action)
    return;

  base::RecordAction(base::UserMetricsAction("MobileTabSwitched"));
}
