// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_content_settings_proxy.h"

#include <memory>
#include <string>

#include "base/memory/ptr_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

ServiceWorkerContentSettingsProxy::ServiceWorkerContentSettingsProxy(
    const GURL origin_url,
    mojom::ServiceWorkerContentSettingsProxyPtrInfo host_info)
    : origin_url_(origin_url), host_info_(std::move(host_info)) {}

ServiceWorkerContentSettingsProxy::~ServiceWorkerContentSettingsProxy() =
    default;

bool ServiceWorkerContentSettingsProxy::RequestFileSystemAccessSync() {
  bool result = false;
  GetInterface()->RequestFileSystemAccessSync(url::Origin(origin_url_),
                                              &result);
  return result;
}

bool ServiceWorkerContentSettingsProxy::AllowIndexedDB(
    const blink::WebString& name,
    const blink::WebSecurityOrigin&) {
  bool result = false;
  GetInterface()->AllowIndexedDB(url::Origin(origin_url_), name.Utf16(),
                                 &result);
  return result;
}

mojom::ServiceWorkerContentSettingsProxyPtr&
ServiceWorkerContentSettingsProxy::GetInterface() {
  static base::ThreadLocalPointer<mojom::ServiceWorkerContentSettingsProxyPtr>*
      host = new base::ThreadLocalPointer<
          mojom::ServiceWorkerContentSettingsProxyPtr>();
  if (host_info_.is_valid()) {
    std::unique_ptr<mojom::ServiceWorkerContentSettingsProxyPtr> ptr =
        base::MakeUnique<mojom::ServiceWorkerContentSettingsProxyPtr>();
    ptr->Bind(std::move(host_info_));
    host->Set(ptr.release());
  }
  return *(host->Get());
}

}  // namespace content
