// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_snapshot_logger.h"

#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_event.pb.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_importance_signals.h"
#include "net/base/mime_util.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

using metrics::TabEvent;

namespace {

constexpr base::TimeDelta kMinimumTimeBetweenTabLogs =
    base::TimeDelta::FromSeconds(10);

// Returns the ContentType that matches |mime_type|.
TabEvent::ContentType GetContentTypeFromMimeType(const std::string& mime_type) {
  // Test for special cases before testing wildcard types.
  if (mime_type.empty())
    return TabEvent::CONTENT_TYPE_UNKNOWN;
  if (net::MatchesMimeType("text/html", mime_type))
    return TabEvent::CONTENT_TYPE_TEXT_HTML;
  if (net::MatchesMimeType("application/*", mime_type))
    return TabEvent::CONTENT_TYPE_APPLICATION;
  if (net::MatchesMimeType("audio/*", mime_type))
    return TabEvent::CONTENT_TYPE_AUDIO;
  if (net::MatchesMimeType("image/*", mime_type))
    return TabEvent::CONTENT_TYPE_IMAGE;
  if (net::MatchesMimeType("text/*", mime_type))
    return TabEvent::CONTENT_TYPE_TEXT;
  if (net::MatchesMimeType("video/*", mime_type))
    return TabEvent::CONTENT_TYPE_VIDEO;
  return TabEvent::CONTENT_TYPE_OTHER;
}

// Returns the site engagement score for the WebContents, rounded down to 10s
// to limit granularity.
int GetSiteEngagementScore(const content::WebContents* web_contents) {
  SiteEngagementService* service = SiteEngagementService::Get(
      Profile::FromBrowserContext(web_contents->GetBrowserContext()));
  DCHECK(service);

  // Scores range from 0 to 100. Round down to a multiple of 10 to conform to
  // privacy guidelines.
  double raw_score = service->GetScore(web_contents->GetVisibleURL());
  int rounded_score = static_cast<int>(raw_score / 10) * 10;
  DCHECK_LE(0, rounded_score);
  DCHECK_GE(100, rounded_score);
  return rounded_score;
}

}  // namespace

TabSnapshotLogger::TabSnapshotLogger() = default;
TabSnapshotLogger::~TabSnapshotLogger() = default;

void TabSnapshotLogger::LogBackgroundTab(ukm::SourceId ukm_source_id,
                                         content::WebContents* web_contents) {
  if (!ukm_source_id)
    return;

  base::TimeTicks& last_tab_log_time = last_log_times_[ukm_source_id];
  base::TimeTicks now = base::TimeTicks::Now();
  if (now - last_tab_log_time < kMinimumTimeBetweenTabLogs)
    return;
  last_tab_log_time = now;

  RecordTabEntry(ukm_source_id, web_contents);
}

void TabSnapshotLogger::RecordTabEntry(ukm::SourceId ukm_source_id,
                                       content::WebContents* web_contents) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (!browser)
    return;

  const TabStripModel* tab_strip_model = browser->tab_strip_model();
  int index = tab_strip_model->GetIndexOfWebContents(web_contents);
  DCHECK_NE(index, TabStripModel::kNoTab);

  ukm::builders::TabManager_Tab tab_entry(ukm_source_id);

  if (SiteEngagementService::IsEnabled())
    tab_entry.SetSiteEngagementScore(GetSiteEngagementScore(web_contents));

  TabEvent::ContentType content_type =
      GetContentTypeFromMimeType(web_contents->GetContentsMimeType());
  tab_entry.SetContentType(static_cast<int>(content_type));
  // TODO(michaelpg): Add PluginType field if mime type matches "application/*"
  // using PluginUMAReporter.

  tab_entry.SetIsPinned(tab_strip_model->IsTabPinned(index))
      .SetHasFormEntry(
          web_contents->GetPageImportanceSignals().had_form_interaction)
      .Record(ukm::UkmRecorder::Get());
}
