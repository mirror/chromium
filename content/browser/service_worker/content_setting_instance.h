// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_CONTENT_SETTING_INSTANCE_H_
#define CONTENT_BROWSER_SERVICE_WORKER_CONTENT_SETTING_INSTANCE_H_

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/content_setting.mojom.h"
#include "url/gurl.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace content {

class CONTENT_EXPORT ContentSettingInstance
    : NON_EXPORTED_BASE(public blink::mojom::ContentSettingInstanceHost) {
 public:
  ~ContentSettingInstance() override;

  static void Create(const service_manager::BindSourceInfo& dource_info,
                     blink::mojom::ContentSettingInstanceHostRequest request);

  void AllowIndexedDB(const GURL& url,
                      const base::string16& name,
                      AllowIndexedDBCallback callback) override;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_CONTENT_SETTING_INSTANCE_H_
