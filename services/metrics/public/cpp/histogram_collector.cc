// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/histogram_collector.h"

#include "base/command_line.h"
#include "base/memory/shared_memory.h"
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
    // NOTE(slan): This will be allocated for any client which didn't pass a
    // |process_type| to the HistogramCollector. This may be a non-content
    // service.
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
      // TODO(slan): UMA metric here?
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

HistogramCollector::~HistogramCollector() {
  // Make sure all of the clients execute their error handlers before this
  // class (specifically the |histogram_provider_|) dies.
  // TODO(slan): Probably better just to move the clients into this class.
}

void HistogramCollector::RegisterClient(
    mojom::HistogramCollectorClientPtr client,
    metrics::CallStackProfileParams::Process process_type,
    mojom::HistogramCollector::RegisterClientCallback cb) {
  // Allocate shared memory for the client to use.
  const uint64_t id = shmem_id_++;
  auto allocator = AllocateSharedMemory(id, process_type);

  // Run the callback before registering this client with the
  // HistogramSynchronizer. This ensures that the client will handle the
  // callback before any data requests are made.
  std::move(cb).Run(allocator
                        ? mojo::WrapSharedMemoryHandle(
                              allocator->shared_memory()->handle().Duplicate(),
                              allocator->shared_memory()->mapped_size(),
                              false /* read_only */)
                        : mojo::ScopedSharedBufferHandle());

  if (allocator) {
    // Register the allocated shared memory with the HistogramProvider...
    auto histogram_allocator =
        base::MakeUnique<base::PersistentHistogramAllocator>(
            std::move(allocator));
    histogram_provider_.RegisterAllocator(id, std::move(histogram_allocator));

    // ...then set the connection_error_handler on the client interface. This
    // will deregister the shared memory if the client tears down their end of
    // the pipe or if this class is destroyed. base::Unretained() is safe here,
    // as the |histogram_provider_| is guaranteed to outlive the
    // |client_bindings_|.
    client.set_connection_error_handler(base::BindRepeating(
        &MetricsServiceHistorgramProvider::DeregisterAllocator,
        base::Unretained(&histogram_provider_), id));
  }

  // Register the client with the HistogramSynchronizer.
  histogram_synchronizer_.RegisterClient(std::move(client));
}

void HistogramCollector::UpdateHistograms(base::TimeDelta wait_time,
                                          UpdateHistogramsCallback callback) {
  // TODO(slan): Plumb this into HistogramSynchronizer so it can be properly
  // run.
  auto bound_callback = base::BindOnce(std::move(callback), false);
  histogram_synchronizer_.FetchHistogramsAsynchronously(
      base::ThreadTaskRunnerHandle::Get(), std::move(bound_callback),
      wait_time);
}

void HistogramCollector::UpdateHistogramsSync(
    HistogramCollector::UpdateHistogramsSyncCallback callback) {
  histogram_synchronizer_.FetchHistograms();
  std::move(callback).Run();
}

}  // namespace metrics
