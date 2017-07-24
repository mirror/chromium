// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_content_settings_proxy_impl.h"

#include "base/threading/thread.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

void ServiceWorkerContentSettingsProxyImpl::Create(
    mojom::ServiceWorkerContentSettingsProxyRequest request,
    base::WeakPtr<ServiceWorkerContextCore> context) {
  mojo::MakeStrongBinding(
      base::WrapUnique(new ServiceWorkerContentSettingsProxyImpl(context)),
      std::move(request));
}

ServiceWorkerContentSettingsProxyImpl::ServiceWorkerContentSettingsProxyImpl(
    base::WeakPtr<ServiceWorkerContextCore> context)
    : context_(context), weak_factory_(this) {}

ServiceWorkerContentSettingsProxyImpl::
    ~ServiceWorkerContentSettingsProxyImpl() = default;

// AllowIndexedDB() and RequestFileSystemAccessSync() pass an empty vector as
// |render_frames| because service worker do not always have frames.
void ServiceWorkerContentSettingsProxyImpl::AllowIndexedDB(
    const GURL& url,
    const base::string16& name,
    AllowIndexedDBCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::move(callback).Run(GetContentClient()->browser()->AllowWorkerIndexedDB(
      url, name, context_->wrapper()->resource_context(),
      std::vector<std::pair<int, int>>()));
}

void ServiceWorkerContentSettingsProxyImpl::RequestFileSystemAccessSync(
    const GURL& url,
    RequestFileSystemAccessSyncCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  GetContentClient()->browser()->AllowWorkerFileSystem(
      url, context_->wrapper()->resource_context(),
      std::vector<std::pair<int, int>>(),
      base::Bind(
          &ServiceWorkerContentSettingsProxyImpl::AllowFileSystemResponce,
          weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void ServiceWorkerContentSettingsProxyImpl::AllowFileSystemResponce(
    base::OnceCallback<void(bool)> callback,
    bool allowed) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::move(callback).Run(allowed);
}

}  // namespace content
