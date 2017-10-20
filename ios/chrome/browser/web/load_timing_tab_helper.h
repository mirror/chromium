// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_LOAD_TIMING_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_LOAD_TIMING_TAB_HELPER_H_

#include "base/macros.h"
#include "base/time/time.h"
#import "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_user_data.h"

// Tracks page load time, measured from when the user presses enter in the
// omnibox to [BrowserViewController tabLoadComplete:withSuccess].
class LoadTimingTabHelper : public web::WebStateUserData<LoadTimingTabHelper> {
 public:
  ~LoadTimingTabHelper() override;

  static void CreateForWebState(web::WebState* web_state);

  // Starts the timer.
  void DidInitiatePageLoad();

  // Timing of promoted prerender tab is not reported.
  void DidPromotePrerenderTab();

  // Stops timer and reports timing if the tab is not a preload tab. Load time
  // for a success preload is essentially zero hence not interesting to track.
  void ReportLoadTime();

  static const char kOmniboxToPageLoadedMetric[];

 private:
  LoadTimingTabHelper();

  bool did_promote_prerender_tab_ = false;
  base::TimeTicks load_start_time_;

  DISALLOW_COPY_AND_ASSIGN(LoadTimingTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_LOAD_TIMING_TAB_HELPER_H_
