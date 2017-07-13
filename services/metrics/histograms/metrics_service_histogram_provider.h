// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_METRICS_SERVICE_HISTOGRAM_PROVIDER_H_
#define SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_METRICS_SERVICE_HISTOGRAM_PROVIDER_H_

#include <string>

#include "base/id_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/metrics/statistics_recorder.h"

namespace metrics {

// MetricsServiceHistorgramProvider gathers and merges histograms stored in
// shared memory segments between HistogramCollectorClients, which may exist in
// remote processes. Merging occurs when a process exits, when metrics are being
// collected for upload, or when something else needs combined metrics (such as
// the chrome://histograms page).
class MetricsServiceHistorgramProvider
    : public base::StatisticsRecorder::HistogramProvider {
 public:
  MetricsServiceHistorgramProvider();
  ~MetricsServiceHistorgramProvider();

  void RegisterAllocator(
      int id,
      std::unique_ptr<base::PersistentHistogramAllocator> allocator);
  void DeregisterAllocator(int id);

 private:
  void MergeHistogramDeltasFromAllocator(
      int id,
      base::PersistentHistogramAllocator* allocator);

  // base::StatisticsRecorder::HistogramProvider implementation.
  void MergeHistogramDeltas() override;

  // All of the shared-persistent-allocators for known clients.
  using AllocatorByIdMap =
      IDMap<std::unique_ptr<base::PersistentHistogramAllocator>, int>;
  AllocatorByIdMap allocators_by_id_;

  base::WeakPtrFactory<MetricsServiceHistorgramProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MetricsServiceHistorgramProvider);
};

}  // namespace metrics

#endif  // SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_METRICS_SERVICE_HISTOGRAM_PROVIDER_H_