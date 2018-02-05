// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_SITE_ENGAGEMENT_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_SITE_ENGAGEMENT_PAGE_LOAD_METRICS_OBSERVER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "components/security_state/core/security_state.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace internal {
extern const char kSiteEngagementHistogram[];
}  // namespace internal

class SiteEngagementPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver,
      public content::WebContentsObserver {
 public:
  static std::unique_ptr<page_load_metrics::PageLoadMetricsObserver>
  MaybeCreateForProfile(content::BrowserContext* profile);

  explicit SiteEngagementPageLoadMetricsObserver(
      content::BrowserContext* context);

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                        const GURL& currently_committed_url,
                        bool started_in_foreground) override;
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;
  ObservePolicy OnHidden(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  ObservePolicy OnShown() override;
  void OnComplete(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;

  // content::WebContentsObserver:
  void DidChangeVisibleSecurityState() override;

 private:
  bool currently_in_foreground_;
  base::TimeDelta foreground_time_;
  base::TimeTicks last_time_shown_;
  double initial_score_;
  SiteEngagementService* engagement_service_ = nullptr;
  SecurityStateTabHelper* security_state_tab_helper_ = nullptr;
  security_state::SecurityLevel initial_security_level_;
  security_state::SecurityLevel current_security_level_;
  ukm::SourceId source_id_;
  content::WebContents* web_contents_ = nullptr;
  GURL current_url_;

  DISALLOW_COPY_AND_ASSIGN(SiteEngagementPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_SITE_ENGAGEMENT_PAGE_LOAD_METRICS_OBSERVER_H_
