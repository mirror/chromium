// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/histogram_controller_client.h"

#include "base/bind.h"
#include "base/metrics/histogram_delta_serialization.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "content/common/histogram.mojom.h"
#include "content/public/common/service_names.mojom.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

HistogramControllerClient::HistogramControllerClient(
    service_manager::Connector* connector)
    : binding_(this) {
  DCHECK(connector);
  mojom::HistogramControllerPtr histogram_controller;
  connector->BindInterface(mojom::kBrowserServiceName,
                           mojo::MakeRequest(&histogram_controller));
  mojom::HistogramControllerClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  histogram_controller->RegisterClient(
      std::move(client),
      base::BindOnce(&HistogramControllerClient::OnClientRegistered,
                     base::Unretained(this)));
}

HistogramControllerClient::~HistogramControllerClient() {}

void HistogramControllerClient::OnClientRegistered(
    mojo::ScopedSharedBufferHandle handle) {
  base::SharedMemoryHandle memory_handle;
  size_t memory_size;
  bool read_only;
  MojoResult result = UnwrapSharedMemoryHandle(
      std::move(handle), &memory_handle, &memory_size, &read_only);

  if (result != MOJO_RESULT_OK)
    return;

  // This message must be received only once. Multiple calls to create a global
  // allocator will cause a CHECK() failure.
  base::GlobalHistogramAllocator::CreateWithSharedMemoryHandle(memory_handle,
                                                               memory_size);
  base::PersistentHistogramAllocator* global_allocator =
      base::GlobalHistogramAllocator::Get();
  if (global_allocator)
    global_allocator->CreateTrackingHistograms(global_allocator->Name());
}

void HistogramControllerClient::RequestNonPersistentHistogramData(
    mojom::HistogramControllerClient::RequestNonPersistentHistogramDataCallback
        callback) {
  // If a persistent allocator is in use, it needs to occasionally update
  // some internal histograms. An upload is happening so this is a good time.
  base::PersistentHistogramAllocator* global_allocator =
      base::GlobalHistogramAllocator::Get();
  if (global_allocator)
    global_allocator->UpdateTrackingHistograms();

  if (!histogram_delta_serialization_) {
    histogram_delta_serialization_ =
        base::MakeUnique<base::HistogramDeltaSerialization>("ChildProcess");
  }

  std::vector<std::string> deltas;
  // "false" to PerpareAndSerializeDeltas() indicates to *not* include
  // histograms held in persistent storage on the assumption that they will be
  // visible to the recipient through other means.
  histogram_delta_serialization_->PrepareAndSerializeDeltas(&deltas, false);
  std::move(callback).Run(deltas);

#ifndef NDEBUG
  static int count = 0;
  count++;
  LOCAL_HISTOGRAM_COUNTS("Histogram.ChildProcessHistogramSentCount", count);
#endif
}

}  // namespace content
