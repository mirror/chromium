// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_CONTENT_SETTING_INSTANCE_H_
#define CONTENT_BROWSER_SERVICE_WORKER_CONTENT_SETTING_INSTANCE_H_

#include "base/callback.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "content/public/browser/resource_context.h"
#include "third_party/WebKit/public/web/shared_worker_content_settings_proxy.mojom.h"
#include "url/gurl.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace content {

class CONTENT_EXPORT SharedWorkerContentSettingsProxyImpl
    : NON_EXPORTED_BASE(public blink::mojom::SharedWorkerContentSettingsProxy) {
 public:
  ~SharedWorkerContentSettingsProxyImpl() override;

  static void Create(
      int render_process_id,
      const service_manager::BindSourceInfo& source_info,
      blink::mojom::SharedWorkerContentSettingsProxyRequest request);

  void AllowIndexedDB(const GURL& url,
                      const base::string16& name,
                      int64_t route_id,
                      AllowIndexedDBCallback callback) override;

  void RequestFileSystemAccessSync(
      const GURL& url,
      int64_t route_id,
      RequestFileSystemAccessSyncCallback callback) override;

 private:
  explicit SharedWorkerContentSettingsProxyImpl(int render_process_id);
  const int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(SharedWorkerContentSettingsProxyImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_CONTENT_SETTING_INSTANCE_H_
