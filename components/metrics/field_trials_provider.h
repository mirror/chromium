// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_FIELD_TRIALS_PROVIDER_H_
#define COMPONENTS_METRICS_FIELD_TRIALS_PROVIDER_H_

#include <vector>

#include "base/time/time.h"
#include "components/metrics/metrics_provider.h"

// TODO(holte): Move this into components/variations once it can depend
// on MetricsProvider/SystemProfileProto
namespace variations {

class SyntheticTrialRegistry;
struct ActiveGroupId;

class FieldTrialsProvider : public metrics::MetricsProvider {
 public:
  // |registry| must outlive this metrics provider.
  FieldTrialsProvider(SyntheticTrialRegistry* registry,
                      base::StringPiece suffix);
  ~FieldTrialsProvider() override;

  // metrics::MetricsProvider:
  void OnDidCreateMetricsLog() override;
  void ProvideSystemProfileMetrics(
      metrics::SystemProfileProto* system_profile_proto) override;

 private:
  // Overrideable for testing.
  virtual void GetFieldTrialIds(
      std::vector<ActiveGroupId>* field_trial_ids) const;

  SyntheticTrialRegistry* registry_;

  //
  std::string suffix_;

  // A stack of log creation times.
  // Emulating current behavior re: initial metrics log.
  std::vector<base::TimeTicks> creation_times_;
};

}  // namespace variations

#endif  // COMPONENTS_METRICS_FIELD_TRIALS_PROVIDER_H_
