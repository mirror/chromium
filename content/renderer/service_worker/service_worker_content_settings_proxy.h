// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_
#define SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_

#include "content/common/service_worker/embedded_worker.mojom.h"
#include "third_party/WebKit/public/platform/WebContentSettingsClient.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebURL.h"

namespace content {

// This proxy is created on the main renderer thread then passed onto
// the blink's worker thread.
class ServiceWorkerContentSettingsProxy
    : public blink::WebContentSettingsClient {
 public:
  ServiceWorkerContentSettingsProxy(
      const blink::WebURL origin_url,
      bool is_unique_origin,
      int route_id,
      mojom::ServiceWorkerContentSettingsProxyPtrInfo content_settings_proxy);
  ~ServiceWorkerContentSettingsProxy() override;

  // WebContentSettingsClient overrides.
  bool RequestFileSystemAccessSync() override;
  bool AllowIndexedDB(const blink::WebString& name,
                      const blink::WebSecurityOrigin&) override;

 private:
  const blink::WebURL origin_url_;
  const bool is_unique_origin_;
  const int route_id_;
  mojom::ServiceWorkerContentSettingsProxyPtrInfo content_settings_proxy_;
  mojom::ServiceWorkerContentSettingsProxyPtr content_settings_;

  // Returns an binded Mojo Ptr.
  mojom::ServiceWorkerContentSettingsProxyPtr& GetService();

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerContentSettingsProxy);
};

}  // namespace content

#endif  // SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_
