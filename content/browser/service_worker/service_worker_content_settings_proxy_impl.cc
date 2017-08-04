// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_content_settings_proxy_impl.h"

#include "base/threading/thread.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/origin.h"

namespace content {

void ServiceWorkerContentSettingsProxyImpl::Create(
    base::WeakPtr<ServiceWorkerContextCore> context,
    mojom::ServiceWorkerContentSettingsProxyRequest request) {
  mojo::MakeStrongBinding(
      base::WrapUnique(new ServiceWorkerContentSettingsProxyImpl(context)),
      std::move(request));
}

ServiceWorkerContentSettingsProxyImpl::ServiceWorkerContentSettingsProxyImpl(
    base::WeakPtr<ServiceWorkerContextCore> context)
    : context_(context), weak_factory_(this) {}

ServiceWorkerContentSettingsProxyImpl::
    ~ServiceWorkerContentSettingsProxyImpl() = default;

void ServiceWorkerContentSettingsProxyImpl::AllowIndexedDB(
    const url::Origin& origin,
    const base::string16& name,
    AllowIndexedDBCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!origin.unique()) {
    // pass an empty vector as |render_frames| because service worker do not
    // always have frames.
    std::move(callback).Run(GetContentClient()->browser()->AllowWorkerIndexedDB(
        origin.GetURL(), name, context_->wrapper()->resource_context(),
        std::vector<std::pair<int, int>>()));
  } else
    std::move(callback).Run(false);
}

void ServiceWorkerContentSettingsProxyImpl::RequestFileSystemAccessSync(
    const url::Origin& origin,
    RequestFileSystemAccessSyncCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!origin.unique()) {
    // pass an empty vector as |render_frames| because service worker do not
    // always have frames.
    GetContentClient()->browser()->AllowWorkerFileSystem(
        origin.GetURL(), context_->wrapper()->resource_context(),
        std::vector<std::pair<int, int>>(),
        base::Bind(
            &ServiceWorkerContentSettingsProxyImpl::AllowFileSystemResponse,
            weak_factory_.GetWeakPtr(), base::Passed(&callback)));
  } else
    std::move(callback).Run(false);
}

void ServiceWorkerContentSettingsProxyImpl::AllowFileSystemResponse(
    base::OnceCallback<void(bool)> callback,
    bool allowed) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::move(callback).Run(allowed);
}

}  // namespace content
