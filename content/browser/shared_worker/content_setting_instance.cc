// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/content_setting_instance.h"
#include <utility>
#include "content/browser/shared_worker/shared_worker_host.h"
#include "content/browser/shared_worker/shared_worker_service_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/gurl.h"

namespace content {

void ContentSettingInstance::Create(
    int render_process_id,
    const service_manager::BindSourceInfo& source_info,
    blink::mojom::ContentSettingInstanceHostRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<ContentSettingInstance>(render_process_id),
      std::move(request));
}

ContentSettingInstance::ContentSettingInstance(int render_process_id)
    : render_process_id_(render_process_id) {}

ContentSettingInstance::~ContentSettingInstance() {}

void ContentSettingInstance::AllowIndexedDB(const GURL& url,
                                            const base::string16& name,
                                            int64_t route_id,
                                            AllowIndexedDBCallback callback) {
  bool result = false;
  SharedWorkerServiceImpl::GetInstance()->AllowIndexedDB(
      render_process_id_, route_id, url, name, &result);
  std::move(callback).Run(result);
}

void ContentSettingInstance::RequestFileSystemAccessSync(
    const GURL& url,
    int64_t route_id,
    RequestFileSystemAccessSyncCallback callback) {
  SharedWorkerServiceImpl::GetInstance()->AllowFileSystem(
      render_process_id_, route_id, url, std::move(callback));
}

}  // namespace content
