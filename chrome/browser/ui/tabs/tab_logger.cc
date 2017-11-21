// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_logger.h"

#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/tabs/tab_event.pb.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_importance_signals.h"
#include "net/base/mime_util.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace tabs {
namespace {

using metrics::TabEvent;

constexpr base::TimeDelta kMinimumTimeBetweenTabLogs =
    base::TimeDelta::FromSeconds(10);

// Returns the ContentType that matches |mime_type|.
int GetContentTypeFromMimeType(const std::string& mime_type) {
  TabEvent::ContentType content_type = TabEvent::CONTENT_TYPE_OTHER;

  // Test for special cases before testing wildcard types.
  if (mime_type.empty())
    content_type = TabEvent::CONTENT_TYPE_UNKNOWN;
  else if (net::MatchesMimeType("text/html", mime_type))
    content_type = TabEvent::CONTENT_TYPE_TEXT_HTML;
  else if (net::MatchesMimeType("application/*", mime_type))
    content_type = TabEvent::CONTENT_TYPE_APPLICATION;
  else if (net::MatchesMimeType("audio/*", mime_type))
    content_type = TabEvent::CONTENT_TYPE_AUDIO;
  else if (net::MatchesMimeType("image/*", mime_type))
    content_type = TabEvent::CONTENT_TYPE_IMAGE;
  else if (net::MatchesMimeType("text/*", mime_type))
    content_type = TabEvent::CONTENT_TYPE_TEXT;
  else if (net::MatchesMimeType("video/*", mime_type))
    content_type = TabEvent::CONTENT_TYPE_VIDEO;
  return static_cast<int>(content_type);
}

// Returns the site engagement score for the WebContents, rounded down to 10s.
int GetSiteEngagementScore(const content::WebContents* contents) {
  SiteEngagementService* service = SiteEngagementService::Get(
      Profile::FromBrowserContext(contents->GetBrowserContext()));
  DCHECK(service);

  // Scores range from 0 to 100. Round down to a multiple of 10.
  double raw_score = service->GetScore(contents->GetVisibleURL());
  int rounded_score = static_cast<int>(raw_score / 10) * 10;
  DCHECK(rounded_score >= 0);
  DCHECK(rounded_score <= 100);
  return rounded_score;
}

}  // namespace

TabLogger::TabLogger() = default;
TabLogger::~TabLogger() = default;

void TabLogger::LogBackgroundTab(const content::WebContents* contents,
                                 const Browser* browser,
                                 const TabStripModel* tab_strip_model,
                                 int index) {
  // Get the NavigationEntry for the visible navigation.
  content::NavigationEntry* entry = contents->GetController().GetVisibleEntry();
  if (!entry)
    return;

  ukm::SourceId ukm_source_id = ukm::ConvertToSourceId(
      entry->GetUniqueID(), ukm::SourceIdType::NAVIGATION_ID);
  if (!ukm_source_id)
    return;

  // TODO(michaelpg): This doesn't throttle events for navigations, as those
  // create new SourceIds.
  const auto last_tab_log_time = last_log_times_.find(ukm_source_id);
  if (last_tab_log_time != last_log_times_.end()) {
    if (base::TimeTicks::Now() - last_tab_log_time->second <
        kMinimumTimeBetweenTabLogs) {
      return;
    }
  }
  last_log_times_[ukm_source_id] = base::TimeTicks::Now();

  RecordTabEntry(ukm_source_id, contents, tab_strip_model, index);
}

void TabLogger::RecordTabEntry(ukm::SourceId ukm_source_id,
                               const content::WebContents* contents,
                               const TabStripModel* tab_strip_model,
                               int index) {
  ukm::builders::TabManager_Tab tab_entry(ukm_source_id);

  if (SiteEngagementService::IsEnabled())
    tab_entry.SetSiteEngagementScore(GetSiteEngagementScore(contents));

  int content_type =
      GetContentTypeFromMimeType(contents->GetContentsMimeType());
  tab_entry.SetContentType(content_type);
  // TODO(michaelpg): Add PluginType field if mime type matches "application/*"
  // using PluginUMAReporter.

  tab_entry.SetIsPinned(tab_strip_model->IsTabPinned(index))
      .SetHasFormEntry(
          contents->GetPageImportanceSignals().had_form_interaction)
      .Record(ukm::UkmRecorder::Get());
}

}  // namespace tabs
