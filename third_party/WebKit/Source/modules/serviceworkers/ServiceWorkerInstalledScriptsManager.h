// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ServiceWorkerInstalledScriptsManager_h
#define ServiceWorkerInstalledScriptsManager_h

#include "core/workers/WorkerInstalledScriptsManager.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

namespace blink {

class ServiceWorkerInstalledScriptsManager
    : public WorkerInstalledScriptsManager {
 public:
  ServiceWorkerInstalledScriptsManager(
      std::unique_ptr<WebServiceWorkerInstalledScriptsManager>);

  // Used on the worker thread.
  Optional<ScriptData> GetScriptData(const KURL& script_url) override;

 private:
  std::unique_ptr<WebServiceWorkerInstalledScriptsManager> manager_;
};

}  // namespace blink

#endif  // WorkerInstalledScriptsManager_h
