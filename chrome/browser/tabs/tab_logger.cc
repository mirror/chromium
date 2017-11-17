// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tabs/tab_logger.h"

#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/tabs/content_type.pb.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/common/page_importance_signals.h"
#include "net/base/mime_util.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace tabs {
namespace {

using metrics::ContentType;

constexpr base::TimeDelta kMinimumTimeBetweenTabLogs =
    base::TimeDelta::FromSeconds(10);

int GetContentTypeFromMimeType(const std::string& mime_type) {
  ContentType content_type = ContentType::OTHER;

  // Test for sub-types before primary types.
  if (net::MatchesMimeType("text/html", mime_type))
    content_type = ContentType::TEXT_HTML;
  else if (net::MatchesMimeType("application/*", mime_type))
    content_type = ContentType::APPLICATION;
  else if (net::MatchesMimeType("audio/*", mime_type))
    content_type = ContentType::AUDIO;
  else if (net::MatchesMimeType("image/*", mime_type))
    content_type = ContentType::IMAGE;
  else if (net::MatchesMimeType("text/*", mime_type))
    content_type = ContentType::TEXT;
  else if (net::MatchesMimeType("video/*", mime_type))
    content_type = ContentType::VIDEO;
  return static_cast<int>(content_type);
}

}  // namespace

TabLogger::TabLogger() = default;
TabLogger::~TabLogger() = default;

void TabLogger::LogTab(
    const resource_coordinator::TabManager::WebContentsData* web_contents_data,
    const TabStripModel* tab_strip_model,
    int tab_index) {
  const ukm::SourceId& ukm_source_id = web_contents_data->ukm_source_id();
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

  content::WebContents* web_contents = web_contents_data->web_contents();
  ukm::builders::TabManager_Tab(ukm_source_id)
      .SetIsPinned(tab_strip_model->IsTabPinned(tab_index))
      .SetHasFormEntry(
          web_contents->GetPageImportanceSignals().had_form_interaction)
      .SetContentType(
          GetContentTypeFromMimeType(web_contents->GetContentsMimeType()))
      .Record(ukm::UkmRecorder::Get());
}

}  // namespace tabs
