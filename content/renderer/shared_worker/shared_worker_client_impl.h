// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SHARED_WORKER_SHARED_WORKER_CLIENT_IMPL_H_
#define CONTENT_RENDERER_SHARED_WORKER_SHARED_WORKER_CLIENT_IMPL_H_

#include "base/macros.h"
#include "content/common/shared_worker/shared_worker_client.mojom.h"
#include "third_party/WebKit/public/web/WebSharedWorkerConnectListener.h"

namespace content {

// XXX - A proxy between a renderer process hosting a document and the browser
// process. This instance has the same lifetime as a shared worker, and
// self-destructs when the worker is destroyed.
class SharedWorkerClientImpl : public mojom::SharedWorkerClient {
 public:
  SharedWorkerClientImpl(
      std::unique_ptr<blink::WebSharedWorkerConnectListener> listener);
  ~SharedWorkerClientImpl() override;

 private:
  // mojom::SharedWorkerClient methods:
  void OnCreated(blink::mojom::SharedWorkerCreationContextType
                     creation_context_type) override;
  void OnConnected(
      const std::vector<blink::mojom::WebFeature>& features_used) override;
  void OnScriptLoadFailed() override;
  void OnFeatureUsed(blink::mojom::WebFeature feature) override;

  std::unique_ptr<blink::WebSharedWorkerConnectListener> listener_;

  DISALLOW_COPY_AND_ASSIGN(SharedWorkerClientImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SHARED_WORKER_SHARED_WORKER_CLIENT_IMPL_H_
