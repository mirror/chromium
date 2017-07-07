// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/gpu/content_gpu_client.h"

#include "content/common/content_client_shutdown_helper.h"

namespace content {

ContentGpuClient::~ContentGpuClient() {
  ContentClientShutdownHelper::ContentClientPartDeleted(this);
}

gpu::SyncPointManager* ContentGpuClient::GetSyncPointManager() {
  return nullptr;
}

}  // namespace content
