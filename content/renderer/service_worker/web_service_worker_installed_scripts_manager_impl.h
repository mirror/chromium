// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_
#define CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_

#include <map>

#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "content/common/service_worker/service_worker_installed_scripts_manager.mojom.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

namespace content {

class WebServiceWorkerInstalledScriptsManagerImpl
    : NON_EXPORTED_BASE(public blink::WebServiceWorkerInstalledScriptsManager) {
 public:
  static std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager> Create(
      mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);

  ~WebServiceWorkerInstalledScriptsManagerImpl() override;

  // Initialize the internal states.
  // Called on the worker thread.
  void WorkerThreadLaunched();

  bool IsScriptInstalled(const blink::WebURL& script_url) const override;

  // Returns the script if it's installed.
  // Called on the worker thread.
  std::unique_ptr<RawScriptData> GetRawScriptData(
      const blink::WebURL& script_url) override;

  // Called from internally on the io thread.
  void FinishScriptTransfer(const GURL& script_url,
                            std::unique_ptr<RawScriptData> script_data);

 private:
  class ScriptData;

  WebServiceWorkerInstalledScriptsManagerImpl(std::vector<GURL> installed_urls);

  // mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info_;
  std::vector<GURL> installed_urls_;

  std::unique_ptr<ScriptData> script_data_;

  // scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  // scoped_refptr<base::SingleThreadTaskRunner> worker_task_runner_;

  // mojo::Binding<ServiceWorkerInstalledScriptsManager> binding_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_
