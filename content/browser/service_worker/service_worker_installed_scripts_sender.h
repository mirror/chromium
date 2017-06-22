// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_

#include "content/common/service_worker/service_worker_installed_scripts_manager.mojom.h"

namespace content {

class ServiceWorkerInstalledScriptsSender {
 public:
  ServiceWorkerInstalledScriptsSender();
  ~ServiceWorkerInstalledScriptsSender();

  mojom::ServiceWorkerInstalledScriptsInfoPtr CreateAndBind();

 private:
  mojom::ServiceWorkerInstalledScriptsManagerPtr manager_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
