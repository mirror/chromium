// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_content_settings_proxy_impl.h"

#include <utility>

#include "content/browser/shared_worker/shared_worker_service_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/gurl.h"

namespace content {

void SharedWorkerContentSettingsProxyImpl::Create(
    base::WeakPtr<SharedWorkerHost> host,
    blink::mojom::SharedWorkerContentSettingsProxyRequest request) {
  mojo::MakeStrongBinding(
      base::WrapUnique(new SharedWorkerContentSettingsProxyImpl(host)),
      std::move(request));
}

SharedWorkerContentSettingsProxyImpl::SharedWorkerContentSettingsProxyImpl(
    base::WeakPtr<SharedWorkerHost> host)
    : host_(host) {}

SharedWorkerContentSettingsProxyImpl::~SharedWorkerContentSettingsProxyImpl() =
    default;

void SharedWorkerContentSettingsProxyImpl::AllowIndexedDB(
    const GURL& url,
    const base::string16& name,
    AllowIndexedDBCallback callback) {
  bool result = host_->AllowIndexedDB(url, name);
  std::move(callback).Run(result);
}

void SharedWorkerContentSettingsProxyImpl::RequestFileSystemAccessSync(
    const GURL& url,
    RequestFileSystemAccessSyncCallback callback) {
  host_->AllowFileSystem(url, std::move(callback));
}

}  // namespace content
