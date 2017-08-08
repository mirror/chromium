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
    GURL script_url,
    base::WeakPtr<ServiceWorkerContextCore> context,
    blink::mojom::WorkerContentSettingsProxyRequest request) {
  mojo::MakeStrongBinding(
      base::WrapUnique(new ServiceWorkerContentSettingsProxyImpl(
          url::Origin(script_url), context)),
      std::move(request));
}

ServiceWorkerContentSettingsProxyImpl::ServiceWorkerContentSettingsProxyImpl(
    url::Origin origin,
    base::WeakPtr<ServiceWorkerContextCore> context)
    : origin_(origin), context_(context) {}

ServiceWorkerContentSettingsProxyImpl::
    ~ServiceWorkerContentSettingsProxyImpl() = default;

void ServiceWorkerContentSettingsProxyImpl::AllowIndexedDB(
    const url::Origin& origin,
    const base::string16& name,
    AllowIndexedDBCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!origin_.unique()) {
    // |render_frames| is used to show UI for the frames affected by the
    // content setting. However, service worker is not necessarily associated
    // with frames or making the request on behalf of frames,
    // so just pass an empty |render_frames|.
    std::vector<std::pair<int, int>> render_frames =
        std::vector<std::pair<int, int>>();
    std::move(callback).Run(GetContentClient()->browser()->AllowWorkerIndexedDB(
        origin.GetURL(), name, context_->wrapper()->resource_context(),
        render_frames));
  } else {
    std::move(callback).Run(false);
  }
}

void ServiceWorkerContentSettingsProxyImpl::RequestFileSystemAccessSync(
    const url::Origin& origin,
    RequestFileSystemAccessSyncCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();
  std::move(callback).Run(false);
}

}  // namespace content
