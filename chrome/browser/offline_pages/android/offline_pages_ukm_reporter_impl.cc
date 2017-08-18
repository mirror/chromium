// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/android/offline_pages_ukm_reporter_impl.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

class GURL;

namespace {
const char kOfflinePageLoadUkmEventName[] = "OfflinePages";
const char kSavePageRequested[] = "SavePageRequested";
}  // namespace

namespace offline_pages {

void OfflinePagesUkmReporterImpl::ReportUrlOfflined(const GURL& gurl) {
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
  // TODO(revivewers) - Do I really need to add this metric?  I don't care about
  // the value, just the source URL.
  builder->AddMetric(kSavePageRequested, 1);

  DVLOG(0) << "@@@@@@ Added metric " << gurl << " " << __func__;
}

}  // namespace offline_pages
