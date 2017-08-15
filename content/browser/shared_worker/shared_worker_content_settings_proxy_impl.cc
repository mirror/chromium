// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_content_settings_proxy_impl.h"

#include <utility>

#include "content/browser/shared_worker/shared_worker_host.h"
#include "content/browser/shared_worker/shared_worker_service_impl.h"
#include "url/gurl.h"

namespace content {

SharedWorkerContentSettingsProxyImpl::SharedWorkerContentSettingsProxyImpl(
    GURL script_url,
    SharedWorkerHost* host,
    blink::mojom::WorkerContentSettingsProxyRequest request)
    : origin_(url::Origin(script_url)), host_(host), binding_(this) {
  binding_.Bind(std::move(request));
}

SharedWorkerContentSettingsProxyImpl::~SharedWorkerContentSettingsProxyImpl() =
    default;

void SharedWorkerContentSettingsProxyImpl::AllowIndexedDB(
    const base::string16& name,
    AllowIndexedDBCallback callback) {
  if (!origin_.unique() && host_) {
    bool result = host_->AllowIndexedDB(origin_.GetURL(), name);
    std::move(callback).Run(result);
  } else
    std::move(callback).Run(false);
}

void SharedWorkerContentSettingsProxyImpl::RequestFileSystemAccessSync(
    RequestFileSystemAccessSyncCallback callback) {
  if (!origin_.unique() && host_) {
    host_->AllowFileSystem(origin_.GetURL(), std::move(callback));
  } else
    std::move(callback).Run(false);
}

}  // namespace content
