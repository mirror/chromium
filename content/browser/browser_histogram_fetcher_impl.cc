// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_histogram_fetcher_impl.h"

#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"
#include "content/browser/histogram_controller.h"
#include "content/common/histogram_fetcher.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

BrowserHistogramFetcherImpl::BrowserHistogramFetcherImpl() {}

BrowserHistogramFetcherImpl::~BrowserHistogramFetcherImpl() {}

void BrowserHistogramFetcherImpl::Create(
    content::mojom::BrowserHistogramFetcherRequest request,
    const service_manager::BindSourceInfo& ignored) {
  mojo::MakeStrongBinding(base::MakeUnique<BrowserHistogramFetcherImpl>(),
                          std::move(request));
}

void BrowserHistogramFetcherImpl::GetBrowserHistogram(
    const std::string& name,
    BrowserHistogramCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // Security: Only allow access to browser histograms when running in the
  // context of a test.
  bool using_stats_collection_controller =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kStatsCollectionController);
  if (!using_stats_collection_controller) {
    LOG(ERROR) << "Attempt at reading browser histogram without specifying "
               << "--" << switches::kStatsCollectionController << " switch.";
    return;
  }
  base::HistogramBase* histogram =
      base::StatisticsRecorder::FindHistogram(name);
  std::string histogram_json;
  if (!histogram) {
    histogram_json = "{}";
  } else {
    histogram->WriteJSON(&histogram_json);
  }
  std::move(callback).Run(histogram_json);
}

}  // namespace content
