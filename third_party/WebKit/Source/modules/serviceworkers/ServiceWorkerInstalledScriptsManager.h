// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ServiceWorkerInstalledScriptsManager_h
#define ServiceWorkerInstalledScriptsManager_h

#include "core/workers/WorkerInstalledScriptsManager.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

namespace blink {

// ServiceWorkerInstalledScriptsManager provides the main script and imported
// scripts which are running on the worker thread.
class ServiceWorkerInstalledScriptsManager
    : public WorkerInstalledScriptsManager {
 public:
  ServiceWorkerInstalledScriptsManager(
      std::unique_ptr<WebServiceWorkerInstalledScriptsManager>);

  // Used on the main or worker thread. Returns true if the script has been
  // installed.
  bool IsScriptInstalled(const KURL& script_url) const override;

  // Used on the worker thread. This is possible to return WTF::nullopt when the
  // script has already been served from this manager even though it's
  // installed.
  Optional<ScriptData> GetScriptData(const KURL& script_url) override;

 private:
  std::unique_ptr<WebServiceWorkerInstalledScriptsManager> manager_;
};

}  // namespace blink

#endif  // WorkerInstalledScriptsManager_h
