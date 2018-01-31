// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"

#ifndef CHROME_BROWSER_METRICS_TESTING_METRICS_REPORTING_PREF_HELPER_H_
#define CHROME_BROWSER_METRICS_TESTING_METRICS_REPORTING_PREF_HELPER_H_

class MetricsReportingPrefsHelper {
 public:
  MetricsReportingPrefsHelper();
  virtual ~MetricsReportingPrefsHelper();

  // ConfiguresSet local state so that that value for reporting is |is_enabled|.
  bool SetUpUserDataDirectory(bool is_enabled);

  base::FilePath getLocalStatePath();

 private:
#if defined(OS_CHROMEOS)
  // Set the pref in |local_state_dict| related to metrics reporting to
  // |is_enabled|. This is only for ChromeOS.
  void SetMetricsReportingEnabledChromeOS(
      bool is_enabled,
      base::DictionaryValue* local_state_dict);
#endif

  base::FilePath local_state_path_;

  DISALLOW_COPY_AND_ASSIGN(MetricsReportingPrefsHelper);
};

#endif  // CHROME_BROWSER_METRICS_TESTING_METRICS_REPORTING_PREF_HELPER_H_
