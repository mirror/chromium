// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_content_settings_proxy.h"

#include <memory>
#include <string>

#include "url/gurl.h"

namespace content {

ServiceWorkerContentSettingsProxy::ServiceWorkerContentSettingsProxy(
    const blink::WebURL origin_url,
    bool is_unique_origin,
    mojom::ServiceWorkerContentSettingsProxyPtrInfo host_info)
    : origin_url_(origin_url),
      is_unique_origin_(is_unique_origin),
      host_info_(std::move(host_info)),
      host_(nullptr) {}

ServiceWorkerContentSettingsProxy::~ServiceWorkerContentSettingsProxy() =
    default;

bool ServiceWorkerContentSettingsProxy::RequestFileSystemAccessSync() {
  if (is_unique_origin_)
    return false;
  bool result = false;
  GetInterface()->RequestFileSystemAccessSync(GURL(origin_url_), &result);
  return result;
}

bool ServiceWorkerContentSettingsProxy::AllowIndexedDB(
    const blink::WebString& name,
    const blink::WebSecurityOrigin&) {
  if (is_unique_origin_)
    return false;
  bool result = false;
  GetInterface()->AllowIndexedDB(GURL(origin_url_), name.Utf16(), &result);
  return result;
}

mojom::ServiceWorkerContentSettingsProxyPtr&
ServiceWorkerContentSettingsProxy::GetInterface() {
  if (!host_) {
    DCHECK(host_info_.is_valid());
    mojom::ServiceWorkerContentSettingsProxyPtr ptr;
    ptr.Bind(std::move(host_info_));
    host_ = std::move(ptr);
  }
  return host_;
}

}  // namespace content
