// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/histograms/metrics_service_histogram_provider.h"

#include "base/metrics/histogram_macros.h"

namespace metrics {

MetricsServiceHistorgramProvider::MetricsServiceHistorgramProvider()
    : weak_ptr_factory_(this) {
  base::StatisticsRecorder::RegisterHistogramProvider(
      weak_ptr_factory_.GetWeakPtr());
}

MetricsServiceHistorgramProvider::~MetricsServiceHistorgramProvider() {}

void MetricsServiceHistorgramProvider::RegisterAllocator(
    int id,
    std::unique_ptr<base::PersistentHistogramAllocator> allocator) {
  DCHECK(!allocators_by_id_.Lookup(id));
  DCHECK(allocator);
  allocators_by_id_.AddWithID(std::move(allocator), id);
}

void MetricsServiceHistorgramProvider::DeregisterAllocator(int id) {
  if (!allocators_by_id_.Lookup(id))
    return;

  // Extract the matching allocator from the list of active ones. It will
  // be automatically released when this method exits.
  std::unique_ptr<base::PersistentHistogramAllocator> allocator(
      allocators_by_id_.Replace(id, nullptr));
  allocators_by_id_.Remove(id);
  DCHECK(allocator);

  // Merge the last deltas from the allocator before it is released.
  MergeHistogramDeltasFromAllocator(id, allocator.get());
}

void MetricsServiceHistorgramProvider::MergeHistogramDeltasFromAllocator(
    int id,
    base::PersistentHistogramAllocator* allocator) {
  DCHECK(allocator);

  int histogram_count = 0;
  base::PersistentHistogramAllocator::Iterator hist_iter(allocator);
  while (true) {
    std::unique_ptr<base::HistogramBase> histogram = hist_iter.GetNext();
    if (!histogram)
      break;
    allocator->MergeHistogramDeltaToStatisticsRecorder(histogram.get());
    ++histogram_count;
  }

  DVLOG(1) << "Reported " << histogram_count << " histograms from client #"
           << id;
}

void MetricsServiceHistorgramProvider::MergeHistogramDeltas() {
  for (AllocatorByIdMap::iterator iter(&allocators_by_id_); !iter.IsAtEnd();
       iter.Advance()) {
    MergeHistogramDeltasFromAllocator(iter.GetCurrentKey(),
                                      iter.GetCurrentValue());
  }

  UMA_HISTOGRAM_COUNTS_100("UMA.SubprocessMetricsProvider.SubprocessCount",
                           allocators_by_id_.size());
}

}  // namespace metrics
