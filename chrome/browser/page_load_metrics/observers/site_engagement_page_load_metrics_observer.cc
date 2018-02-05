// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/site_engagement_page_load_metrics_observer.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/engagement/site_engagement_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "components/security_state/core/security_state.h"
#include "content/public/browser/render_frame_host.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace internal {
// Change in Site Engagement scores split by SecurityLevel.
const char kEngagementDeltaNoneHistogram[] =
    "Security.SiteEngagementDelta.NONE";
const char kEngagementDeltaHttpShowWarningHistogram[] =
    "Security.SiteEngagementDelta.HTTP_SHOW_WARNING";
const char kEngagementDeltaEvSecureHistogram[] =
    "Security.SiteEngagementDelta.EV_SECURE";
const char kEngagementDeltaSecureHistogram[] =
    "Security.SiteEngagementDelta.SECURE";
const char kEngagementDeltaPolicyInstalledCertHistogram[] =
    "Security.SiteEngagementDelta.POLICY_INSTALLED_CERT";
const char kEngagementDeltaDangerousHistogram[] =
    "Security.SiteEngagementDelta.DANGEROUS";

// Final Site Engagement score split by SecurityLevel.
const char kEngagementFinalNoneHistogram[] = "Security.SiteEngagement.NONE";
const char kEngagementFinalHttpShowWarningHistogram[] =
    "Security.SiteEngagement.HTTP_SHOW_WARNING";
const char kEngagementFinalEvSecureHistogram[] =
    "Security.SiteEngagement.EV_SECURE";
const char kEngagementFinalSecureHistogram[] = "Security.SiteEngagement.SECURE";
const char kEngagementFinalPolicyInstalledCertHistogram[] =
    "Security.SiteEngagement.POLICY_INSTALLED_CERT";
const char kEngagementFinalDangerousHistogram[] =
    "Security.SiteEngagement.DANGEROUS";
}  // namespace internal

// static
std::unique_ptr<page_load_metrics::PageLoadMetricsObserver>
SiteEngagementPageLoadMetricsObserver::MaybeCreateForProfile(
    content::BrowserContext* profile) {
  if (!SiteEngagementService::IsEnabled())
    return nullptr;
  return std::make_unique<SiteEngagementPageLoadMetricsObserver>(profile);
}

SiteEngagementPageLoadMetricsObserver::SiteEngagementPageLoadMetricsObserver(
    content::BrowserContext* profile)
    : content::WebContentsObserver(),
      currently_in_foreground_(false),
      initial_score_(0.0) {
  engagement_service_ = SiteEngagementServiceFactory::GetForProfile(
      static_cast<Profile*>(profile));
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SiteEngagementPageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  initial_score_ = engagement_service_->GetScore(navigation_handle->GetURL());
  if (started_in_foreground)
    OnShown();
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SiteEngagementPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  // Only monitor navigations committed to the main frame.
  if (!navigation_handle->IsInMainFrame())
    return STOP_OBSERVING;

  source_id_ = source_id;
  current_url_ = navigation_handle->GetURL();

  web_contents_ = navigation_handle->GetWebContents();
  if (!web_contents_)
    return STOP_OBSERVING;
  Observe(web_contents_);

  // Gather initial security level after all server redirects have been
  // resolved.
  security_state_tab_helper_ =
      SecurityStateTabHelper::FromWebContents(web_contents_);
  security_state::SecurityInfo security_info;
  security_state_tab_helper_->GetSecurityInfo(&security_info);
  initial_security_level_ = security_info.security_level;
  current_security_level_ = initial_security_level_;
  source_id_ = source_id;
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SiteEngagementPageLoadMetricsObserver::OnHidden(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  if (currently_in_foreground_) {
    foreground_time_ += base::TimeTicks::Now() - last_time_shown_;
    currently_in_foreground_ = false;
  }
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SiteEngagementPageLoadMetricsObserver::OnShown() {
  last_time_shown_ = base::TimeTicks::Now();
  currently_in_foreground_ = true;
  return CONTINUE_OBSERVING;
}

void SiteEngagementPageLoadMetricsObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  if (!extra_info.did_commit || !extra_info.url.is_valid()) {
    return;
  }

  // Don't record anything if the user never saw it.
  if (!currently_in_foreground_ && foreground_time_.is_zero())
    return;

  double final_score = engagement_service_->GetScore(extra_info.url);

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Security_SiteEngagement(source_id_)
      .SetInitialSecurityLevel(initial_security_level_)
      .SetFinalSecurityLevel(current_security_level_)
      .SetInitialScore(initial_score_)
      .SetFinalScore(final_score)
      .Record(ukm_recorder);

  double delta = final_score - initial_score_;
  switch (current_security_level_) {
    case security_state::NONE:
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementDeltaNoneHistogram, delta);
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementFinalNoneHistogram,
                               final_score);
      break;
    case security_state::HTTP_SHOW_WARNING:
      UMA_HISTOGRAM_COUNTS_100(
          internal::kEngagementDeltaHttpShowWarningHistogram, delta);
      UMA_HISTOGRAM_COUNTS_100(
          internal::kEngagementFinalHttpShowWarningHistogram, final_score);
      break;
    case security_state::EV_SECURE:
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementDeltaEvSecureHistogram,
                               delta);
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementFinalEvSecureHistogram,
                               final_score);
      break;
    case security_state::SECURE:
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementDeltaSecureHistogram,
                               delta);
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementFinalSecureHistogram,
                               final_score);
      break;
    case security_state::SECURE_WITH_POLICY_INSTALLED_CERT:
      UMA_HISTOGRAM_COUNTS_100(
          internal::kEngagementDeltaPolicyInstalledCertHistogram, delta);
      UMA_HISTOGRAM_COUNTS_100(
          internal::kEngagementFinalPolicyInstalledCertHistogram, final_score);
      break;
    case security_state::DANGEROUS:
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementDeltaDangerousHistogram,
                               delta);
      UMA_HISTOGRAM_COUNTS_100(internal::kEngagementFinalDangerousHistogram,
                               final_score);
      break;
    default:
      break;
  }
}

void SiteEngagementPageLoadMetricsObserver::DidChangeVisibleSecurityState() {
  if (security_state_tab_helper_ &&
      web_contents_->GetMainFrame()->GetLastCommittedURL() == current_url_) {
    security_state::SecurityInfo security_info;
    security_state_tab_helper_->GetSecurityInfo(&security_info);
    current_security_level_ = security_info.security_level;
  }
}
