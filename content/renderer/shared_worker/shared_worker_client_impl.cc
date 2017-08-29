// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/shared_worker/shared_worker_client_impl.h"

#include "content/child/child_thread_impl.h"
#include "content/child/webmessageportchannel_impl.h"
#include "content/common/shared_worker/shared_worker.mojom.h"
#include "content/common/view_messages.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/WebKit/public/platform/web_feature.mojom.h"

namespace content {

SharedWorkerClientImpl::SharedWorkerClientImpl(
    std::unique_ptr<blink::WebSharedWorkerConnectListener> listener)
    : listener_(std::move(listener)) {}

SharedWorkerClientImpl::~SharedWorkerClientImpl() {}

void SharedWorkerClientImpl::OnCreated(uint32_t error_code) {
  listener_->WorkerCreated(
      static_cast<blink::WebWorkerCreationError>(error_code));
}

void SharedWorkerClientImpl::OnConnected(
    const std::vector<blink::mojom::WebFeature>& features_used) {
  listener_->Connected();
  for (auto feature : features_used)
    listener_->CountFeature(static_cast<uint32_t>(feature));
}

void SharedWorkerClientImpl::OnScriptLoadFailed() {
  listener_->ScriptLoadFailed();
}

void SharedWorkerClientImpl::OnFeatureUsed(blink::mojom::WebFeature feature) {
  listener_->CountFeature(static_cast<uint32_t>(feature));
}

}  // namespace content
