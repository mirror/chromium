// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_histogram_provider.h"

#include "base/command_line.h"
#include "base/metrics/statistics_recorder.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"

namespace content {

BrowserHistogramProvider::BrowserHistogramProvider() {}

BrowserHistogramProvider::~BrowserHistogramProvider() {}

void BrowserHistogramProvider::GetHistogram(const std::string& name,
                                            GetHistogramCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Security: Only allow access to browser histograms when running in the
  // context of a test.
  bool using_stats_collection_controller =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kStatsCollectionController);
  if (!using_stats_collection_controller) {
    LOG(ERROR) << "Attempt at reading browser histogram without specifying "
               << "--" << switches::kStatsCollectionController << " switch.";
    std::move(callback).Run(base::nullopt);
    return;
  }
  base::HistogramBase* histogram =
      base::StatisticsRecorder::FindHistogram(name);

  std::string histogram_json = "{}";
  if (histogram)
    histogram->WriteJSON(&histogram_json);
  std::move(callback).Run(histogram_json);
}

}  // namespace content
