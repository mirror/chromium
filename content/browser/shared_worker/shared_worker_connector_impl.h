// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONNECTOR_IMPL_H_
#define CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONNECTOR_IMPL_H_

#include "content/browser/shared_worker/worker_storage_partition.h"
#include "content/common/shared_worker/shared_worker_connector.mojom.h"

namespace content {
class ResourceContext;

class SharedWorkerConnectorImpl : public mojom::SharedWorkerConnector {
 public:
  // Called on the UI thread:
  static void Create(int process_id,
                     int frame_id,
                     mojom::SharedWorkerConnectorRequest request);

 private:
  static void CreateOnIOThread(int process_id,
                               int frame_id,
                               ResourceContext* resource_context,
                               WorkerStoragePartition partition,
                               mojom::SharedWorkerConnectorRequest request);

  SharedWorkerConnectorImpl(
      int process_id,
      int frame_id,
      ResourceContext* resource_context,
      const WorkerStoragePartition& worker_storage_partition);

  // mojom::SharedWorkerConnector methods:
  void Connect(mojom::SharedWorkerInfoPtr info,
               mojom::SharedWorkerClientPtr client,
               mojo::ScopedMessagePipeHandle message_port) override;

  int process_id_;
  int frame_id_;
  ResourceContext* resource_context_;
  WorkerStoragePartition worker_storage_partition_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONNECTOR_IMPL_H_
