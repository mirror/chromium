// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_
#define CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_

#include <map>

#include "content/common/service_worker/service_worker_installed_scripts_manager.mojom.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

namespace content {

class WebServiceWorkerInstalledScriptsManagerImpl
    : public mojom::ServiceWorkerInstalledScriptsManager,
      NON_EXPORTED_BASE(public blink::WebServiceWorkerInstalledScriptsManager) {
 public:
  static std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager> Create(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);

  ~WebServiceWorkerInstalledScriptsManagerImpl() override;

  // Called on the io thread.
  void InitializeOnIO();

  // Implements mojom::ServiceWorkerInstalledScriptsManager.
  // Called on the io thread.
  void TransferInstalledScript(
      const GURL& script_url,
      uint32_t body_size,
      mojo::ScopedDataPipeConsumerHandle body,
      uint32_t meta_data_size,
      mojo::ScopedDataPipeConsumerHandle meta_data) override;

  // Initialize the internal states.
  // Called on the worker thread.
  void WorkerThreadLaunched();

  // Returns the script if it's installed.
  // Called on the worker thread.
  std::unique_ptr<RawScriptData> GetRawScriptData(
      const blink::WebURL& script_url) override;

 private:
  WebServiceWorkerInstalledScriptsManagerImpl(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> worker_task_runner_;

  std::map<GURL, std::unique_ptr<RawScriptData>> meta_data;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_
