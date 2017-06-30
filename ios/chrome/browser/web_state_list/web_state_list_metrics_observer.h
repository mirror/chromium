// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_LIST_WEB_STATE_LIST_METRICS_OBSERVER_H_
#define IOS_CHROME_BROWSER_WEB_STATE_LIST_WEB_STATE_LIST_METRICS_OBSERVER_H_

#include "base/macros.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"

class TabUsageRecorder;

class WebStateListMetricsObserver : public WebStateListObserver {
 public:
  explicit WebStateListMetricsObserver(TabUsageRecorder* tab_usage_recorder);
  ~WebStateListMetricsObserver() override;

  void ResetSessionMetrics();
  void RecordSessionMetrics();

  // WebStateListObserver implementation.
  void WebStateInsertedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index,
                          bool foreground) override;
  void WebStateReplacedAt(WebStateList* web_state_list,
                          web::WebState* old_web_state,
                          web::WebState* new_web_state,
                          int index) override;
  void WebStateDetachedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index) override;
  void WebStateActivatedAt(WebStateList* web_state_list,
                           web::WebState* old_web_state,
                           web::WebState* new_web_state,
                           int active_index,
                           bool user_action) override;

 private:
  // Counters for metrics.
  int inserted_web_state_counter_;
  int detached_web_state_counter_;
  int activated_web_state_counter_;

  // Tab usage recorder.
  TabUsageRecorder* tab_usage_recorder_;

  DISALLOW_COPY_AND_ASSIGN(WebStateListMetricsObserver);
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_LIST_WEB_STATE_LIST_METRICS_OBSERVER_H_
