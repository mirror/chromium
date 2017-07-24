// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
#define SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_

#include "base/memory/weak_ptr.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/embedded_worker.mojom.h"

namespace content {

class ServiceWorkerContextCore;

class ServiceWorkerContentSettingsProxyImpl
    : public mojom::ServiceWorkerContentSettingsProxy {
 public:
  ~ServiceWorkerContentSettingsProxyImpl() override;

  // Create a new ServiceWorkerContentSettingsProxyImpl
  // and binds it to |request|.
  static void Create(mojom::ServiceWorkerContentSettingsProxyRequest request,
                     base::WeakPtr<ServiceWorkerContextCore> context);

  // mojom::ServiceWorkerContentSettingsProxy implementation
  // These pass an empty vector as |render_frames| because service worker do
  // not always have frames.
  void AllowIndexedDB(const GURL& url,
                      const base::string16& name,
                      int64_t route_id,
                      AllowIndexedDBCallback callback) override;
  void RequestFileSystemAccessSync(
      const GURL& url,
      int64_t route_id,
      RequestFileSystemAccessSyncCallback callback) override;

 private:
  ServiceWorkerContentSettingsProxyImpl(
      base::WeakPtr<ServiceWorkerContextCore> context);

  // This is used for returning the result of RequestFileSystemAccessSync().
  void AllowFileSystemResponce(base::OnceCallback<void(bool)> callback,
                               bool allowed);

  base::WeakPtr<ServiceWorkerContextCore> context_;
  base::WeakPtrFactory<ServiceWorkerContentSettingsProxyImpl> weak_factory_;
};

}  // namespace content

#endif  // SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
