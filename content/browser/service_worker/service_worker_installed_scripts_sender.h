// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_

#include <map>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/common/service_worker/service_worker_installed_scripts_manager.mojom.h"

namespace content {

// ServiceWorkerInstalledScriptsSender serves the service worker's installed
// scripts from ServiceWorkerStorage to the renderer through Mojo's pipes.
class ServiceWorkerInstalledScriptsSender {
 public:
  ServiceWorkerInstalledScriptsSender();
  ~ServiceWorkerInstalledScriptsSender();

  // Creates a Mojo's struct (mojom::ServiceWorkerInstalledScriptsInfo) and sets
  // the information to create WebServiceWorkerInstalledScriptsManager on the
  // renderer.
  mojom::ServiceWorkerInstalledScriptsInfoPtr CreateInfoAndBind();

 private:
  mojom::ServiceWorkerInstalledScriptsManagerPtr manager_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
