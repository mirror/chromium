// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerClients.h"

namespace blink {

WorkerClients::WorkerClients(
    std::unique_ptr<WebWorkerContentSettingsClientProxy> proxy)
    : proxy_(std::move(proxy)) {}

bool WorkerClients::RequestFileSystemAccessSync() {
  if (!proxy_)
    return true;
  return proxy_->RequestFileSystemAccessSync();
}

bool WorkerClients::AllowIndexedDB(const WebString& name) {
  if (!proxy_)
    return true;
  return proxy_->AllowIndexedDB(name);
}

template class CORE_TEMPLATE_EXPORT Supplement<WorkerClients>;

}  // namespace blink
