// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LOCAL_STATE_LOADER_H_
#define CHROME_BROWSER_LOCAL_STATE_LOADER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "build/build_config.h"

class ChromeBrowserFieldTrials;
class ChromeMetricsServicesManagerClient;
class FieldTrialSynchronizer;
class PrefService;

namespace base {
class FieldTrialList;
class SequencedTaskRunner;
}  // namespace base

#if !defined(OS_ANDROID)
namespace first_run {
struct MasterPrefs;
}
#endif

namespace metrics_services_manager {
class MetricsServicesManager;
}

namespace policy {
class ChromeBrowserPolicyConnector;
}

namespace prefs {
class InProcessPrefServiceFactory;
}

namespace local_state_loader {

// Members needed to fully initialize local state.
struct LocalStateData {
  LocalStateData();
  ~LocalStateData();

  scoped_refptr<base::SequencedTaskRunner> local_state_task_runner;
  std::unique_ptr<prefs::InProcessPrefServiceFactory> pref_service_factory;
  std::unique_ptr<PrefService> local_state;
  std::unique_ptr<policy::ChromeBrowserPolicyConnector>
      browser_policy_connector;
#if !defined(OS_ANDROID)
  std::unique_ptr<first_run::MasterPrefs> master_prefs;
#endif
  std::unique_ptr<metrics_services_manager::MetricsServicesManager>
      metrics_services_manager;
  std::unique_ptr<base::FieldTrialList> field_trial_list;
  scoped_refptr<FieldTrialSynchronizer> field_trial_synchronizer;
  std::unique_ptr<ChromeBrowserFieldTrials> browser_field_trials;
  // This is owned by |metrics_services_manager|.
  ChromeMetricsServicesManagerClient* metrics_services_manager_client = nullptr;
};

// Return value is a chrome::ResultCode. RESULT_CODE_NORMAL_EXIT indicates
// success.
int LoadLocalState(LocalStateData* data);

namespace internal {

// Called during location to do a portion of initialization. This is exposed
// here for tests and should not be needed outside of tests.
void CreateLocalState(
    scoped_refptr<base::SequencedTaskRunner> local_state_task_runner,
    std::unique_ptr<prefs::InProcessPrefServiceFactory>* pref_service_factory,
    std::unique_ptr<policy::ChromeBrowserPolicyConnector>*
        browser_policy_connector,
    std::unique_ptr<PrefService>* local_state);

}  // namespace internal

}  // namespace local_state_loader

#endif  // CHROME_BROWSER_LOCAL_STATE_LOADER_H_
