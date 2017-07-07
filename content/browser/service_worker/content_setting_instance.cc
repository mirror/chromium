// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/content_setting_instance.h"

#include <utility>
#include "base/logging.h"

namespace content {

ContentSettingInstance::~ContentSettingInstance() {}

void ContentSettingInstance::Create(
    const service_manager::BindSourceInfo& source_info,
    blink::mojom::ContentSettingInstanceHostRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<ContentSettingInstance>(),
                          std::move(request));
}

void ContentSettingInstance::AllowIndexedDB(const GURL& url,
                                            const base::string16& name,
                                            AllowIndexedDBCallback callback) {
  // *result = GetContentClient()->browser()->AllowWorkerIndexedDB(
  //   url, name, instance_->resource_context(), GetRenderFrameIDsForWorker());
  std::move(callback).Run(true);
  LOG(ERROR) << __PRETTY_FUNCTION__ << "---------- test ----------";
}

}  // namespace content
