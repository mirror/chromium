// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/histogram_collector.h"

#include "base/command_line.h"
#include "base/memory/shared_memory.h"
#include "base/metrics/histogram_delta_serialization.h"
#include "base/metrics/histogram_macros.h"
#include "components/metrics/public/interfaces/call_stack_profile_collector.mojom.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace metrics {
namespace {

// Allocate shared memory and create a persistent memory allocator based on the
// |process_type| received from the client.
std::unique_ptr<base::SharedPersistentMemoryAllocator> AllocateSharedMemory(
    uint64_t id,
    CallStackProfileParams::Process process_type) {
  size_t memory_size;
  std::string metrics_name;

  switch (process_type) {
    case CallStackProfileParams::Process::UNKNOWN_PROCESS:
      memory_size = 64 << 10;  // 64 KiB
      metrics_name = "UnknownMetrics";
      break;

    case CallStackProfileParams::Process::BROWSER_PROCESS:
      NOTREACHED() << "Browser Process not supported!";
      return nullptr;

    case CallStackProfileParams::Process::RENDERER_PROCESS:
      memory_size = 2 << 20;  // 2 MiB
      metrics_name = "RendererMetrics";
      break;

    case CallStackProfileParams::Process::GPU_PROCESS:
      memory_size = 64 << 10;  // 64 KiB
      metrics_name = "GpuMetrics";
      break;

    case CallStackProfileParams::Process::UTILITY_PROCESS:
      memory_size = 64 << 10;  // 64 KiB
      metrics_name = "UtilityMetrics";
      break;

    case CallStackProfileParams::Process::ZYGOTE_PROCESS:
      memory_size = 64 << 10;  // 64 KiB
      metrics_name = "ZygoteMetrics";
      break;

    case CallStackProfileParams::Process::SANDBOX_HELPER_PROCESS:
      memory_size = 64 << 10;  // 64 KiB
      metrics_name = "SandboxHelperMetrics";
      break;

    case CallStackProfileParams::Process::PPAPI_PLUGIN_PROCESS:
      memory_size = 64 << 10;  // 64 KiB
      metrics_name = "PpapiPluginMetrics";
      break;

    case CallStackProfileParams::Process::PPAPI_BROKER_PROCESS:
      memory_size = 64 << 10;  // 64 KiB
      metrics_name = "PpapiBrokerMetrics";
      break;

    default:
      return nullptr;
  }

  // Create the shared memory segment and attach an allocator to it.
  // Mapping the memory shouldn't fail but be safe if it does; everything
  // will continue to work but just as if persistence weren't available.
  auto shm = base::MakeUnique<base::SharedMemory>();
  if (!shm->CreateAndMapAnonymous(memory_size))
    return nullptr;

  return base::MakeUnique<base::SharedPersistentMemoryAllocator>(
      std::move(shm), id, metrics_name, /*readonly=*/false);
}

}  // namespace

HistogramCollector::HistogramCollector() {}

HistogramCollector::~HistogramCollector() {}

void HistogramCollector::RegisterClient(
    mojom::HistogramCollectorClientPtr client,
    metrics::CallStackProfileParams::Process process_type,
    mojom::HistogramCollector::RegisterClientCallback cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Allocate shared memory for the client to use.
  ClientId client_id = client.get();
  auto allocator =
      AllocateSharedMemory(reinterpret_cast<uint64_t>(client_id), process_type);
  auto shmem_handle =
      allocator ? mojo::WrapSharedMemoryHandle(
                      allocator->shared_memory()->handle().Duplicate(),
                      allocator->shared_memory()->mapped_size(),
                      false /* read_only */)
                : mojo::ScopedSharedBufferHandle();
  if (allocator) {
    // Register the allocated shared memory with the HistogramProvider...
    auto histogram_allocator =
        base::MakeUnique<base::PersistentHistogramAllocator>(
            std::move(allocator));
    histogram_provider_.RegisterAllocator(reinterpret_cast<uint64_t>(client_id),
                                          std::move(histogram_allocator));
  }

  // ...then set the connection_error_handler on the client interface. This
  // will deregister the shared memory if the client tears down their end of
  // the pipe or if this class is destroyed. base::Unretained() is safe here,
  // as |this| is guaranteed to outlive all of the |clients_|.
  client.set_connection_error_handler(
      base::BindRepeating(&HistogramCollector::OnClientConnectionDestroyed,
                          base::Unretained(this), client_id));

  clients_.AddPtr(std::move(client));
  active_client_ids_.insert(client_id);
  std::move(cb).Run(std::move(shmem_handle));
}

void HistogramCollector::OnClientConnectionDestroyed(ClientId client_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Remove the allocator associated with this client. This call is valid even
  // if there was no valid allocator created.
  histogram_provider_.DeregisterAllocator(reinterpret_cast<size_t>(client_id));

  // Remove the client from the set of active clients.
  active_client_ids_.erase(client_id);

  // Remove the client from the list of clients from which callbacks are
  // expected. There are many situations where this client id may not be in
  // this set. For example, if the client was Registered() after the request was
  // issued, if the client already responded, or if there is no request pending.
  pending_client_ids_.erase(client_id);
}

void HistogramCollector::UpdateHistograms(base::TimeDelta wait_time,
                                          UpdateHistogramsCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // If there is a pending callback, post this callback with |complete| = false.
  // The pending request is superseded by the current request; all responses
  // from clients associated with the pending request will be ignored.
  RunPendingCallback(false /* complete */);
  pending_callback_ = std::move(callback);

  // Get a new sequence number for the upcoming batch of client requests.
  ++last_used_sequence_number_;
  if (last_used_sequence_number_ < 0) {
    last_used_sequence_number_ = 1;
  }

  // Set the timeout callback.
  timer_.Start(FROM_HERE, wait_time,
               base::Bind(&HistogramCollector::OnWaitTimeExpired,
                          base::Unretained(this), last_used_sequence_number_));

  // Issue a request to all clients to update their histograms, using the new
  // sequence number.
  RequestHistogramUpdatesFromAllClients(last_used_sequence_number_);
}

void HistogramCollector::RequestHistogramUpdatesFromAllClients(
    int sequence_number) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  pending_client_ids_.clear();
  clients_.ForAllPtrs([this, sequence_number](
                          metrics::mojom::HistogramCollectorClient* client) {
    client->RequestNonPersistentHistogramData(
        base::BindOnce(&HistogramCollector::OnHistogramDataCollected,
                       base::Unretained(this), client, sequence_number));
    pending_client_ids_.insert(client);
  });
}

void HistogramCollector::OnHistogramDataCollected(
    ClientId client_id,
    int sequence_number,
    const std::vector<std::string>& pickled_histograms) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Ignore the request if the sequence id is old.
  if (sequence_number != last_used_sequence_number_)
    return;

  base::HistogramDeltaSerialization::DeserializeAndAddSamples(
      pickled_histograms);

  // Remove the id from the pending clients.
  pending_client_ids_.erase(client_id);
  if (pending_client_ids_.empty()) {
    timer_.Stop();
    RunPendingCallback(true /* complete*/);
  }
}

void HistogramCollector::OnWaitTimeExpired(int sequence_number) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // If |sequence_number| doesn't match |last_used_sequence_number_|, this is an
  // old timeout - another request has already been issued. Silently ignore this
  // callback.
  if (sequence_number != last_used_sequence_number_)
    return;

  // Alert the caller that the request timed out before it could finish
  // successfully.
  RunPendingCallback(false /* complete */);
}

void HistogramCollector::RunPendingCallback(bool complete) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!pending_callback_)
    return;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(pending_callback_), complete));
}

}  // namespace metrics
