// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_H_
#define SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_H_

#include <string>

#include "base/id_map.h"
#include "base/macros.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/metrics/persistent_memory_allocator.h"
#include "base/metrics/statistics_recorder.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/metrics/histograms/histogram_synchronizer.h"
#include "services/metrics/public/interfaces/histogram.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace metrics {

// This class provides histograms to remote services.
class HistogramCollector : public mojom::HistogramCollector {
 public:
  HistogramCollector();
  ~HistogramCollector() override;

  // Bind this interface to a request.
  void Bind(const service_manager::BindSourceInfo& source_info,
            mojom::HistogramCollectorRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // mojom::HistogramCollector implementation.
  void RegisterClient(
      mojom::HistogramCollectorClientPtr client,
      metrics::CallStackProfileParams::Process process_type,
      mojom::HistogramCollector::RegisterClientCallback cb) override;
  void UpdateHistograms(base::TimeDelta wait_time,
                        UpdateHistogramsCallback callback) override;
  void UpdateHistogramsSync(UpdateHistogramsSyncCallback callback) override;

 private:
  uint64_t shmem_id_ = 0u;

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

    //  base::StatisticsRecorder::HistogramProvider implementation.
    void MergeHistogramDeltas() override;

    // All of the shared-persistent-allocators for known clients.
    using AllocatorByIdMap =
        IDMap<std::unique_ptr<base::PersistentHistogramAllocator>, int>;
    AllocatorByIdMap allocators_by_id_;

    base::WeakPtrFactory<MetricsServiceHistorgramProvider> weak_ptr_factory_;

    DISALLOW_COPY_AND_ASSIGN(MetricsServiceHistorgramProvider);
  };

  MetricsServiceHistorgramProvider histogram_provider_;
  HistogramSynchronizer histogram_synchronizer_;
  mojo::BindingSet<mojom::HistogramCollector> bindings_;

  DISALLOW_COPY_AND_ASSIGN(HistogramCollector);
};

}  // namespace metrics

#endif  // SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_H_
