// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_SYSTEM_PROFILE_WRITER_H_
#define COMPONENTS_METRICS_SYSTEM_PROFILE_WRITER_H_

#include "base/macros.h"
#include "components/metrics/metrics_service_client.h"
#include "components/metrics/proto/chrome_user_metrics_extension.pb.h"
#include "components/variations/active_field_trials.h"

namespace metrics {

class SystemProfileWriter {
 public:
  SystemProfileWriter();

  virtual ~SystemProfileWriter();

  // Get the GMT buildtime for the current binary, expressed in seconds since
  // January 1, 1970 GMT.
  // The value is used to identify when a new build is run, so that previous
  // reliability stats, from other builds, can be abandoned.
  static int64_t GetBuildTime();

  // Record core profile settings into the SystemProfileProto.
  void RecordCoreSystemProfile(MetricsServiceClient* client,
                               SystemProfileProto* system_profile) const;

  void RecordCPU(SystemProfileProto* system_profile) const;

  // Write the default state of the enable metrics checkbox.
  void WriteMetricsEnableDefault(bool is_reporting_policy_managed,
                                 EnableMetricsDefault metrics_default,
                                 SystemProfileProto* system_profile) const;

  void WriteVariationsFieldTrials(SystemProfileProto* system_profile) const;

  void WriteFieldTrials(
      const std::vector<variations::ActiveGroupId>& field_trial_ids,
      SystemProfileProto* system_profile) const;

 protected:
  // Exposed for the sake of mocking/accessing in test code.
  // Fills |field_trial_ids| with the list of initialized field trials name and
  // group ids.
  virtual void GetFieldTrialIds(
      std::vector<variations::ActiveGroupId>* field_trial_ids) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemProfileWriter);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_SYSTEM_PROFILE_WRITER_H_
