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
    int render_process_id,
    blink::mojom::SharedWorkerContentSettingsProxyRequest request) {
  mojo::MakeStrongBinding(
      base::WrapUnique(
          new SharedWorkerContentSettingsProxyImpl(render_process_id)),
      std::move(request));
}

SharedWorkerContentSettingsProxyImpl::SharedWorkerContentSettingsProxyImpl(
    int render_process_id)
    : render_process_id_(render_process_id) {}

SharedWorkerContentSettingsProxyImpl::~SharedWorkerContentSettingsProxyImpl() =
    default;

void SharedWorkerContentSettingsProxyImpl::AllowIndexedDB(
    const GURL& url,
    const base::string16& name,
    int64_t route_id,
    AllowIndexedDBCallback callback) {
  bool result = SharedWorkerServiceImpl::GetInstance()->AllowIndexedDB(
      render_process_id_, route_id, url, name);
  std::move(callback).Run(result);
}

void SharedWorkerContentSettingsProxyImpl::RequestFileSystemAccessSync(
    const GURL& url,
    int64_t route_id,
    RequestFileSystemAccessSyncCallback callback) {
  SharedWorkerServiceImpl::GetInstance()->AllowFileSystem(
      render_process_id_, route_id, url, std::move(callback));
}

}  // namespace content
