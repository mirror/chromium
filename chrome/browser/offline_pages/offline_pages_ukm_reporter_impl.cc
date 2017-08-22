// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/offline_pages_ukm_reporter_impl.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "url/gurl.h"

namespace {
const char kOfflinePageLoadUkmEventName[] = "OfflinePages";
const char kSavePageRequested[] = "SavePageRequested";
}  // namespace

namespace offline_pages {

void OfflinePagesUkmReporterImpl::ReportUrlOfflined(const GURL& gurl,
                                                    bool foreground) {
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (ukm_recorder == nullptr)
    return;

  // This unique ID represents this whole navigation.
  int32_t source_id = ukm::UkmRecorder::GetNewSourceID();

  // Associate the URL with this navigation.
  ukm_recorder->UpdateSourceURL(source_id, gurl);

  // Tag this metric as an offline page request for the URL.  This is a private
  // member of UkmRecorder, so we need to be friends to use it.
  std::unique_ptr<ukm::UkmEntryBuilder> builder =
      ukm_recorder->GetEntryBuilder(source_id, kOfflinePageLoadUkmEventName);
  int metric_value = 0;
  if (foreground)
    metric_value = 1;
  builder->AddMetric(kSavePageRequested, metric_value);
}

}  // namespace offline_pages
