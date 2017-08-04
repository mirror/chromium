// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_

#include "base/memory/weak_ptr.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/embedded_worker.mojom.h"

namespace content {

class ServiceWorkerContextCore;

class ServiceWorkerContentSettingsProxyImpl final
    : public mojom::ServiceWorkerContentSettingsProxy {
 public:
  ~ServiceWorkerContentSettingsProxyImpl() override;

  // Creates ServiceWorkerContentSettingsProxyImpl and binds it to |request|.
  // This creates a StrongBinding, so the lifetime of the new
  // ServiceWorkerContentSettingsProxyImpl is the same as its Mojo connection.
  static void Create(base::WeakPtr<ServiceWorkerContextCore> context,
                     mojom::ServiceWorkerContentSettingsProxyRequest request);

  // mojom::ServiceWorkerContentSettingsProxy implementation
  void AllowIndexedDB(const url::Origin& origin,
                      const base::string16& name,
                      AllowIndexedDBCallback callback) override;
  void RequestFileSystemAccessSync(
      const url::Origin& origin,
      RequestFileSystemAccessSyncCallback callback) override;

 private:
  explicit ServiceWorkerContentSettingsProxyImpl(
      base::WeakPtr<ServiceWorkerContextCore> context);

  // This is used for returning the result of RequestFileSystemAccessSync().
  void AllowFileSystemResponse(base::OnceCallback<void(bool)> callback,
                               bool allowed);

  base::WeakPtr<ServiceWorkerContextCore> context_;
  base::WeakPtrFactory<ServiceWorkerContentSettingsProxyImpl> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
