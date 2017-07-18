// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SharedWorkerContentSettingsProxy_h
#define SharedWorkerContentSettingsProxy_h

#include "public/platform/WebContentSettingsClient.h"
#include "public/platform/WebSecurityOrigin.h"
#include "public/web/shared_worker_content_settings_proxy.mojom-blink.h"

namespace blink {

// SharedWorkerContentSettingsProxy provides content settings information.
// This class is created on the main thread and then called on the IO thread.
// Each information is requested via a Mojo connection to the browser process.
// The connection is established on each method call and then destructed.
// This is assumed to be used only by shared worker.
class SharedWorkerContentSettingsProxy : public WebContentSettingsClient {
 public:
  SharedWorkerContentSettingsProxy(const KURL& origin_url,
                                   bool is_unique_origin,
                                   int route_id);
  ~SharedWorkerContentSettingsProxy() override;

  // WebContentSettingsClient overrides.
  bool AllowIndexedDB(const WebString& name, const WebSecurityOrigin&) override;
  bool RequestFileSystemAccessSync() override;

 private:
  // |origin_url_| is set on the main thread at the constructor,
  // so please use it with KURL::Copy() in each method.
  const KURL origin_url_;
  const bool is_unique_origin_;
  const int route_id_;
  // mojom::blink::SharedWorkerContentSettingsProxyPtr
  // content_setting_instance_host_;

  // Returns an initialized SharedWorkerContentSettingsProxyPtr.
  // Each call to this method establishes a new Mojo connection,
  // because the returned pointer can only be used in the thread which made it.
  mojom::blink::SharedWorkerContentSettingsProxyPtr GetService();
};

}  // namespace blink

#endif  // SharedWorkerContentSettingsProxy_h
