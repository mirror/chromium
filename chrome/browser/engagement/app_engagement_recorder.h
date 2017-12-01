// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENGAGEMENT_APP_ENGAGEMENT_RECORDER_H_
#define CHROME_BROWSER_ENGAGEMENT_APP_ENGAGEMENT_RECORDER_H_

#include "chrome/browser/engagement/site_engagement_observer.h"

namespace content {
class WebContents;
}

class GURL;
class SiteEngagementService;

// TODO(mgiuca): Move out of this directory? DO NOT SUBMIT.
class AppEngagementRecorder : public SiteEngagementObserver {
 public:
  explicit AppEngagementRecorder(SiteEngagementService* service);
  ~AppEngagementRecorder() override;

  // SiteEngagementObserver overrides.
  void OnEngagementEvent(content::WebContents* web_contents,
                         const GURL& url,
                         double score,
                         SiteEngagementService::EngagementType type) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppEngagementRecorder);
};

#endif  // CHROME_BROWSER_ENGAGEMENT_APP_ENGAGEMENT_RECORDER_H_
