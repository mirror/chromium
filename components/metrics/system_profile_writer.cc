// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/system_profile_writer.h"
#include "base/build_time.h"
#include "base/cpu.h"
#include "base/sys_info.h"

namespace metrics {

namespace {}  // end namespace

SystemProfileWriter::SystemProfileWriter() {}

SystemProfileWriter::~SystemProfileWriter() {}

// static
int64_t SystemProfileWriter::GetBuildTime() {
  static int64_t integral_build_time = 0;
  if (!integral_build_time)
    integral_build_time = static_cast<int64_t>(base::GetBuildTime().ToTimeT());
  return integral_build_time;
}

void SystemProfileWriter::RecordCoreSystemProfile(
    MetricsServiceClient* client,
    SystemProfileProto* system_profile) const {
  system_profile->set_build_timestamp(GetBuildTime());
  system_profile->set_app_version(client->GetVersionString());
  system_profile->set_channel(client->GetChannel());
  system_profile->set_application_locale(client->GetApplicationLocale());

#if defined(SYZYASAN)
  system_profile->set_is_asan_build(true);
#endif

  auto* hardware = system_profile->mutable_hardware();
#if !defined(OS_IOS)
  // On iOS, OperatingSystemArchitecture() returns values like iPad4,4 which is
  // not the actual CPU architecture. Don't set it until the API is fixed. See
  // crbug.com/370104 for details.
  hardware->set_cpu_architecture(base::SysInfo::OperatingSystemArchitecture());
#endif
  hardware->set_system_ram_mb(base::SysInfo::AmountOfPhysicalMemoryMB());
  hardware->set_hardware_class(base::SysInfo::HardwareModelName());
#if defined(OS_WIN)
  hardware->set_dll_base(reinterpret_cast<uint64_t>(CURRENT_MODULE()));
#endif

  metrics::SystemProfileProto::OS* os = system_profile->mutable_os();
  os->set_name(base::SysInfo::OperatingSystemName());
  os->set_version(base::SysInfo::OperatingSystemVersion());
#if defined(OS_ANDROID)
  os->set_fingerprint(
      base::android::BuildInfo::GetInstance()->android_build_fp());
#endif
}

void SystemProfileWriter::RecordCPU(SystemProfileProto* system_profile) const {
  auto* cpu = system_profile->mutable_hardware()->mutable_cpu();
  base::CPU cpu_info;
  cpu->set_vendor_name(cpu_info.vendor_name());
  cpu->set_signature(cpu_info.signature());
  cpu->set_num_cores(base::SysInfo::NumberOfProcessors());
}

void SystemProfileWriter::WriteMetricsEnableDefault(
    bool is_reporting_policy_managed,
    EnableMetricsDefault metrics_default,
    SystemProfileProto* system_profile) const {
  if (is_reporting_policy_managed) {
    // If it's managed, then it must be reporting, otherwise we wouldn't be
    // sending metrics.
    system_profile->set_uma_default_state(
        SystemProfileProto_UmaDefaultState_POLICY_FORCED_ENABLED);
    return;
  }

  switch (metrics_default) {
    case EnableMetricsDefault::DEFAULT_UNKNOWN:
      // Don't set the field if it's unknown.
      break;
    case EnableMetricsDefault::OPT_IN:
      system_profile->set_uma_default_state(
          SystemProfileProto_UmaDefaultState_OPT_IN);
      break;
    case EnableMetricsDefault::OPT_OUT:
      system_profile->set_uma_default_state(
          SystemProfileProto_UmaDefaultState_OPT_OUT);
  }
}

void SystemProfileWriter::WriteVariationsFieldTrials(
    SystemProfileProto* system_profile) const {
  std::vector<variations::ActiveGroupId> field_trial_ids;
  GetFieldTrialIds(&field_trial_ids);
  WriteFieldTrials(field_trial_ids, system_profile);
}

void SystemProfileWriter::WriteFieldTrials(
    const std::vector<variations::ActiveGroupId>& field_trial_ids,
    SystemProfileProto* system_profile) const {
  for (auto it = field_trial_ids.begin(); it != field_trial_ids.end(); ++it) {
    SystemProfileProto::FieldTrial* field_trial =
        system_profile->add_field_trial();
    field_trial->set_name_id(it->name);
    field_trial->set_group_id(it->group);
  }
}

void SystemProfileWriter::GetFieldTrialIds(
    std::vector<variations::ActiveGroupId>* field_trial_ids) const {
  variations::GetFieldTrialActiveGroupIds(field_trial_ids);
}

}  // namespace metrics
