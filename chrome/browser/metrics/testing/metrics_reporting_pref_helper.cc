// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/testing/metrics_reporting_pref_helper.h"

#include "base/files/file_path.h"
#include "base/json/json_file_value_serializer.h"
#include "base/path_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_service_accessor.h"
#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/settings/device_settings_cache.h"
#include "components/policy/proto/chrome_device_policy.pb.h"
#include "components/policy/proto/device_management_backend.pb.h"
#endif

MetricsReportingPrefsHelper::MetricsReportingPrefsHelper() {}

MetricsReportingPrefsHelper::~MetricsReportingPrefsHelper() {}

bool MetricsReportingPrefsHelper::SetUpUserDataDirectory(bool is_enabled) {
  base::DictionaryValue local_state_dict;
  local_state_dict.SetBoolean(metrics::prefs::kMetricsReportingEnabled,
                              is_enabled);

  base::FilePath user_data_dir;
  if (!PathService::Get(chrome::DIR_USER_DATA, &user_data_dir))
    return false;

#if defined(OS_CHROMEOS)
  // ChromeOS checks a separate place for reporting enabled.
  SetMetricsReportingEnabledChromeOS(
      is_metrics_reporting_enabled_initial_value(), &local_state_dict);
#endif

  local_state_path_ = user_data_dir.Append(chrome::kLocalStateFilename);
  return JSONFileValueSerializer(local_state_path_).Serialize(local_state_dict);
}

base::FilePath MetricsReportingPrefsHelper::getLocalStatePath() {
  return local_state_path_;
}

#if defined(OS_CHROMEOS)
void MetricsReportingPrefsHelper::SetMetricsReportingEnabledChromeOS(
    bool is_enabled,
    base::DictionaryValue* local_state_dict) {
  namespace em = enterprise_management;
  em::ChromeDeviceSettingsProto device_settings_proto;
  device_settings_proto.mutable_metrics_enabled()->set_metrics_enabled(
      is_enabled);
  em::PolicyData policy_data;
  policy_data.set_policy_type("google/chromeos/device");
  policy_data.set_policy_value(device_settings_proto.SerializeAsString());
  local_state_dict->SetString(
      prefs::kDeviceSettingsCache,
      chromeos::device_settings_cache::PolicyDataToString(policy_data));
}
#endif
