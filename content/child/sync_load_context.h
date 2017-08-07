// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_SYNC_LOAD_CONTEXT_H_
#define CONTENT_CHILD_SYNC_LOAD_CONTEXT_H_

#include "base/macros.h"
#include "content/child/resource_dispatcher.h"
#include "content/public/child/request_peer.h"
#include "content/public/common/url_loader_factory.mojom.h"

namespace base {
class WaitableEvent;
}

namespace url {
class Origin;
}

namespace content {

struct ResourceRequest;
struct SyncLoadResponse;

class SyncLoadContext : public RequestPeer {
 public:
  static void StartAsyncWithWaitableEvent(
      std::unique_ptr<ResourceRequest> request,
      int routing_id,
      const url::Origin& frame_origin,
      mojom::URLLoaderFactoryPtrInfo url_loader_factory_pipe,
      SyncLoadResponse* response,
      base::WaitableEvent* event);

  SyncLoadContext(ResourceRequest* request,
                  mojom::URLLoaderFactoryPtrInfo url_loader_factory,
                  SyncLoadResponse* response,
                  base::WaitableEvent* event);
  ~SyncLoadContext() override;

 private:
  void OnUploadProgress(uint64_t position, uint64_t size) override;
  bool OnReceivedRedirect(const net::RedirectInfo& redirect_info,
                          const ResourceResponseInfo& info) override;
  void OnReceivedResponse(const ResourceResponseInfo& info) override;
  void OnDownloadedData(int len, int encoded_data_length) override;
  void OnReceivedData(std::unique_ptr<ReceivedData> data) override;
  void OnTransferSizeUpdated(int transfer_size_diff) override;
  void OnCompletedRequest(int error_code,
                          bool stale_copy_in_cache,
                          const base::TimeTicks& completion_time,
                          int64_t total_transfer_size,
                          int64_t encoded_body_size,
                          int64_t decoded_body_size) override;

  // This raw pointer will remain valid for the lifetime of this object because
  // it remains on the stack until |event_| is signaled.
  SyncLoadResponse* response_;

  int request_id_;

  // State necessary to run a request on an independent thread.
  mojom::URLLoaderFactoryPtr url_loader_factory_;
  std::unique_ptr<ResourceDispatcher> resource_dispatcher_;

  // This event is signaled when the request is complete.
  base::WaitableEvent* event_;

  DISALLOW_COPY_AND_ASSIGN(SyncLoadContext);
};

}  // namespace content

#endif  // CONTENT_CHILD_SYNC_LOAD_CONTEXT_H_
