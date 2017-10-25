// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/first_run_internal.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/first_run/first_run_dialog.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/master_preferences.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "components/prefs/pref_service.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"

namespace first_run {

void (*g_before_show_first_run_dialog_hook_for_testing)();

namespace internal {
namespace {

#if !defined(OS_CHROMEOS)
// Launch the first run dialog only for certain builds, and only if the user has
// not already set preferences.
bool ShouldShowFirstRunDialog(bool* update_metrics_reporting) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kForceFirstRunDialog)) {
    return true;
  }

#if !defined(GOOGLE_CHROME_BUILD)
  // On non-official builds, only --force-first-run-dialog will show the dialog.
  return false;
#endif

  base::FilePath local_state_path;
  PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path);
  if (base::PathExists(local_state_path))
    return false;

  if (!internal::IsOrganicFirstRun())
    return false;

  // The purpose of the dialog is to ask the user to enable stats and crash
  // reporting. This setting may be controlled through configuration management
  // in enterprise scenarios. If that is the case, skip the dialog entirely, as
  // it's not worth bothering the user for only the default browser question
  // (which is likely to be forced in enterprise deployments anyway).
  if (IsMetricsReportingPolicyManaged())
    return false;

  *update_metrics_reporting = true;
  return true;
}
#endif  // !OS_CHROMEOS

}  // namespace

void DoPostImportPlatformSpecificTasks(Profile* profile) {
#if !defined(OS_CHROMEOS)
  bool update_metrics_reporting = false;
  if (!ShouldShowFirstRunDialog(&update_metrics_reporting))
    return;

  if (g_before_show_first_run_dialog_hook_for_testing)
    g_before_show_first_run_dialog_hook_for_testing();

  ShowFirstRunDialog(profile);
  startup_metric_utils::SetNonBrowserUIDisplayed();

  if (update_metrics_reporting) {
    bool is_opt_in = first_run::IsMetricsReportingOptIn();
    metrics::RecordMetricsReportingDefaultState(
        g_browser_process->local_state(),
        is_opt_in ? metrics::EnableMetricsDefault::OPT_IN
                  : metrics::EnableMetricsDefault::OPT_OUT);
  }
#endif  // !OS_CHROMEOS
}

bool IsFirstRunSentinelPresent() {
  base::FilePath sentinel;
  return !GetFirstRunSentinelFilePath(&sentinel) || base::PathExists(sentinel);
}

bool ShowPostInstallEULAIfNeeded(installer::MasterPreferences* install_prefs) {
  // The EULA is only handled on Windows.
  return true;
}

}  // namespace internal
}  // namespace first_run
