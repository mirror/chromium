// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_DELEGATING_PROVIDER_H_
#define COMPONENTS_METRICS_DELEGATING_PROVIDER_H_

#include <memory>
#include <vector>

#include "components/metrics/metrics_provider.h"

namespace metrics {

class DelegatingProvider : public MetricsProvider {
 public:
  DelegatingProvider();
  ~DelegatingProvider() override;

  void RegisterMetricsProvider(std::unique_ptr<MetricsProvider> delegate);
  const std::vector<std::unique_ptr<MetricsProvider>>& GetProviders();

  // MetricsProvider:
  void Init() override;
  void AsyncInit(const base::Closure& done_callback) override;
  void OnDidCreateMetricsLog() override;
  void OnRecordingEnabled() override;
  void OnRecordingDisabled() override;
  void OnAppEnterBackground() override;
  bool ProvideIndependentMetrics(
      SystemProfileProto* system_profile_proto,
      base::HistogramSnapshotManager* snapshot_manager) override;
  void ProvideSystemProfileMetrics(
      SystemProfileProto* system_profile_proto) override;
  bool HasPreviousSessionData() override;
  void ProvidePreviousSessionData(
      ChromeUserMetricsExtension* uma_proto) override;
  void ProvideCurrentSessionData(
      ChromeUserMetricsExtension* uma_proto) override;
  void ClearSavedStabilityMetrics() override;
  void RecordHistogramSnapshots(
      base::HistogramSnapshotManager* snapshot_manager) override;
  void RecordInitialHistogramSnapshots(
      base::HistogramSnapshotManager* snapshot_manager) override;

 private:
  std::vector<std::unique_ptr<MetricsProvider>> metrics_providers_;

  DISALLOW_COPY_AND_ASSIGN(DelegatingProvider);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_DELEGATING_PROVIDER_H_
