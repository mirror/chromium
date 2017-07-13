// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/histogram_collector_client.h"

#include "base/bind.h"
#include "base/metrics/histogram_delta_serialization.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "components/metrics/public/interfaces/call_stack_profile_collector.mojom.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/metrics/public/interfaces/constants.mojom.h"
#include "services/metrics/public/interfaces/histogram.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace metrics {

HistogramCollectorClient::HistogramCollectorClient(
    service_manager::Connector* connector)
    : HistogramCollectorClient(
          connector,
          metrics::CallStackProfileParams::Process::UNKNOWN_PROCESS) {}

HistogramCollectorClient::HistogramCollectorClient(
    service_manager::Connector* connector,
    metrics::CallStackProfileParams::Process process_type)
    : binding_(this) {
  ConnectInternal(connector, process_type);
}

HistogramCollectorClient::~HistogramCollectorClient() {}

void HistogramCollectorClient::ConnectInternal(
    service_manager::Connector* connector,
    metrics::CallStackProfileParams::Process process_type) {
  DCHECK(connector);
  mojom::HistogramCollectorPtr histogram_collector;
  connector->BindInterface(metrics::mojom::kServiceName,
                           mojo::MakeRequest(&histogram_collector));

  // Create an InterfaceRequest and bind it to this object. There's no need to
  // set a connection_error_handler here. If the connection is torn down
  // remotely, this object can't be used again. It can be destroyed whenever.
  mojom::HistogramCollectorClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));

  histogram_collector->RegisterClient(
      std::move(client), process_type,
      base::BindOnce(&HistogramCollectorClient::OnClientRegistered,
                     base::Unretained(this)));
}

void HistogramCollectorClient::OnClientRegistered(
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

void HistogramCollectorClient::RequestNonPersistentHistogramData(
    mojom::HistogramCollectorClient::RequestNonPersistentHistogramDataCallback
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

}  // namespace metrics
