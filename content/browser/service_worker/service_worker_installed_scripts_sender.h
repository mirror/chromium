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

struct HttpResponseInfoIOBuffer;
class ServiceWorkerContextCore;
class ServiceWorkerScriptCacheMap;
class ServiceWorkerVersion;

// ServiceWorkerInstalledScriptsSender serves the service worker's installed
// scripts from ServiceWorkerStorage to the renderer through Mojo's pipes.
class ServiceWorkerInstalledScriptsSender {
 public:
  ServiceWorkerInstalledScriptsSender(
      ServiceWorkerVersion* owner,
      const GURL& main_script_url,
      base::WeakPtr<ServiceWorkerScriptCacheMap> cache_map,
      base::WeakPtr<ServiceWorkerContextCore> context);
  ~ServiceWorkerInstalledScriptsSender();

  // Creates a Mojo's struct (mojom::ServiceWorkerInstalledScriptsInfo) and sets
  // the information to create WebServiceWorkerInstalledScriptsManager on the
  // renderer.
  mojom::ServiceWorkerInstalledScriptsInfoPtr CreateInfoAndBind();

 private:
  class Sender;

  void StartPushScript(int64_t resource_id);

  // Callbacks from |running_sender_|.
  void OnStartScriptTransfer(
      std::string encoding,
      std::unordered_map<std::string, std::string> headers,
      mojo::ScopedDataPipeConsumerHandle meta_data_handle,
      mojo::ScopedDataPipeConsumerHandle body_handle);
  void OnHttpInfoRead(scoped_refptr<HttpResponseInfoIOBuffer> http_info);
  void OnFinishScriptTransfer();
  void OnAbortScriptTransfer();

  bool is_main_script();

  ServiceWorkerVersion* owner_;
  const GURL main_script_url_;

  mojom::ServiceWorkerInstalledScriptsManagerPtr manager_;

  std::unique_ptr<Sender> running_sender_;

  std::map<int64_t /* resource_id */, GURL> imported_urls_;
  std::map<int64_t /* resource_id */, GURL>::iterator current_imported_url_;
  base::WeakPtr<ServiceWorkerScriptCacheMap> cache_map_;
  base::WeakPtr<ServiceWorkerContextCore> context_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
