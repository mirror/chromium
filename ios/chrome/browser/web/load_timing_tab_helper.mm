// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/load_timing_tab_helper.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#import "ios/web/public/web_state/web_state.h"

DEFINE_WEB_STATE_USER_DATA_KEY(LoadTimingTabHelper);

const char LoadTimingTabHelper::kOmniboxToPageLoadedMetric[] =
    "IOS.PageLoadTiming.OmniboxToPageLoaded";

LoadTimingTabHelper::LoadTimingTabHelper() = default;
LoadTimingTabHelper::~LoadTimingTabHelper() = default;

void LoadTimingTabHelper::CreateForWebState(web::WebState* web_state) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(),
                           base::WrapUnique(new LoadTimingTabHelper));
  }
}

void LoadTimingTabHelper::DidInitiatePageLoad() {
  load_start_time_ = base::TimeTicks::Now();
}

void LoadTimingTabHelper::ResetAndReportLoadTime() {
  // load_start_time_ may be null if the current load is not initiated from the
  // omnibox.
  if (!load_start_time_.is_null()) {
    base::TimeDelta load_time = base::TimeTicks::Now() - load_start_time_;
    UMA_HISTOGRAM_TIMES(kOmniboxToPageLoadedMetric, load_time);
    Reset();
  }
}

void LoadTimingTabHelper::Reset() {
  load_start_time_ = base::TimeTicks();
}
