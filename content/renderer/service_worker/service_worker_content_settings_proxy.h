// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_
#define CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_

#include "base/threading/thread_local.h"
#include "content/common/service_worker/embedded_worker.mojom.h"
#include "third_party/WebKit/public/platform/WebContentSettingsClient.h"

namespace content {

// Provides the content settings from browser process.
// This proxy is created and destroyed on the main thread and used on the
// worker thread.
class ServiceWorkerContentSettingsProxy final
    : public blink::WebContentSettingsClient {
 public:
  ServiceWorkerContentSettingsProxy(
      const GURL origin_url,
      mojom::ServiceWorkerContentSettingsProxyPtrInfo host_info);
  ~ServiceWorkerContentSettingsProxy() override;

  // WebContentSettingsClient overrides.
  // Asks the settings to the browser process.
  // Blocks until the result is back to the renderer.
  bool RequestFileSystemAccessSync() override;
  bool AllowIndexedDB(const blink::WebString& name,
                      const blink::WebSecurityOrigin&) override;

 private:
  // Returns a bound Interface Ptr.
  mojom::ServiceWorkerContentSettingsProxyPtr& GetInterface();

  const GURL origin_url_;
  mojom::ServiceWorkerContentSettingsProxyPtrInfo host_info_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerContentSettingsProxy);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_
