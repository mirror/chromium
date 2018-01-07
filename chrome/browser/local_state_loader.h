// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LOCAL_STATE_LOADER_H_
#define CHROME_BROWSER_LOCAL_STATE_LOADER_H_

#include <memory>

#include "base/memory/ref_counted.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}

namespace policy {
class BrowserPolicyConnector;
}

namespace prefs {
class InProcessPrefServiceFactory;
}

namespace local_state_loader {

// chromeos::CrosSettings::Initialize() makes use of g_browser_process. Needs
// to be changed to take what it needs (local_state && browser_policy_connector)
struct LocalState {
  LocalState();
  ~LocalState();

  scoped_refptr<base::SequencedTaskRunner> local_state_task_runner;
  std::unique_ptr<prefs::InProcessPrefServiceFactory> pref_service_factory;
  std::unique_ptr<PrefService> local_state;
  std::unique_ptr<policy::BrowserPolicyConnector> browser_policy_connector;
};

std::unique_ptr<LocalState> CreateAndLoadLocalState();

}  // namespace local_state_loader

#endif  // CHROME_BROWSER_LOCAL_STATE_LOADER_H_
