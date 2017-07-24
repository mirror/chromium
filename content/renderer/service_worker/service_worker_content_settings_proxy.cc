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
    int route_id,
    mojom::ServiceWorkerContentSettingsProxyPtrInfo content_settings_proxy)
    : origin_url_(origin_url),
      is_unique_origin_(is_unique_origin),
      route_id_(route_id),
      content_settings_proxy_(std::move(content_settings_proxy)) {
  RequestFileSystemAccessSync();
}

ServiceWorkerContentSettingsProxy::~ServiceWorkerContentSettingsProxy() =
    default;

bool ServiceWorkerContentSettingsProxy::RequestFileSystemAccessSync() {
  if (is_unique_origin_)
    return false;
  bool result = false;
  GetService()->RequestFileSystemAccessSync(GURL(origin_url_), route_id_,
                                            &result);
  return result;
}

bool ServiceWorkerContentSettingsProxy::AllowIndexedDB(
    const blink::WebString& name,
    const blink::WebSecurityOrigin&) {
  if (is_unique_origin_)
    return false;
  bool result = false;
  GetService()->AllowIndexedDB(GURL(origin_url_), name.Utf16(), route_id_,
                               &result);
  return result;
}

mojom::ServiceWorkerContentSettingsProxyPtr&
ServiceWorkerContentSettingsProxy::GetService() {
  if (!content_settings_ && content_settings_proxy_.is_valid()) {
    mojom::ServiceWorkerContentSettingsProxyPtr ptr;
    ptr.Bind(std::move(content_settings_proxy_));
    content_settings_ = std::move(ptr);
  }
  return content_settings_;
}

}  // namespace content
