// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/coordinator_impl.h"

#include <inttypes.h>
#include <stdio.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/pattern.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/client_process_impl.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/constants.mojom.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"
#include "services/service_manager/public/cpp/identity.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include "base/mac/mac_util.h"
#endif

using base::trace_event::MemoryDumpLevelOfDetail;
using base::trace_event::MemoryDumpType;

namespace memory_instrumentation {

namespace {

memory_instrumentation::CoordinatorImpl* g_coordinator_impl;

// Returns the private memory footprint calculated from given |os_dump|.
//
// See design docs linked in the bugs for the rationale of the computation:
// - Linux/Android: https://crbug.com/707019 .
// - Mac OS: https://crbug.com/707021 .
// - Win: https://crbug.com/707022 .
uint32_t CalculatePrivateFootprintKb(const mojom::RawOSMemDump& os_dump) {
  DCHECK(os_dump.platform_private_footprint);
#if defined(OS_LINUX) || defined(OS_ANDROID)
  uint64_t rss_anon_bytes = os_dump.platform_private_footprint->rss_anon_bytes;
  uint64_t vm_swap_bytes = os_dump.platform_private_footprint->vm_swap_bytes;
  return (rss_anon_bytes + vm_swap_bytes) / 1024;
#elif defined(OS_MACOSX)
  // TODO(erikchen): This calculation is close, but not fully accurate. It
  // overcounts by anonymous shared memory.
  if (base::mac::IsAtLeastOS10_12()) {
    uint64_t phys_footprint_bytes =
        os_dump.platform_private_footprint->phys_footprint_bytes;
    return phys_footprint_bytes / 1024;
  } else {
    uint64_t internal_bytes =
        os_dump.platform_private_footprint->internal_bytes;
    uint64_t compressed_bytes =
        os_dump.platform_private_footprint->compressed_bytes;
    return (internal_bytes + compressed_bytes) / 1024;
  }
#elif defined(OS_WIN)
  return os_dump.platform_private_footprint->private_bytes / 1024;
#else
  return 0;
#endif
}

memory_instrumentation::mojom::OSMemDumpPtr CreatePublicOSDump(
    const mojom::RawOSMemDump& internal_os_dump) {
  mojom::OSMemDumpPtr os_dump = mojom::OSMemDump::New();

  os_dump->resident_set_kb = internal_os_dump.resident_set_kb;
  os_dump->private_footprint_kb = CalculatePrivateFootprintKb(internal_os_dump);
  return os_dump;
}

uint32_t GetDumpsSumKb(const std::string& pattern,
                       const base::trace_event::ProcessMemoryDump& pmd) {
  uint64_t sum = 0;
  for (const auto& kv : pmd.allocator_dumps()) {
    auto name = base::StringPiece(kv.first);
    if (base::MatchPattern(name, pattern)) {
      sum += kv.second->GetSizeInternal();
    }
  }
  return sum / 1024;
}

mojom::ChromeMemDumpPtr CreateDumpSummary(
    const base::trace_event::ProcessMemoryDump& process_memory_dump) {
  mojom::ChromeMemDumpPtr result = mojom::ChromeMemDump::New();
  // TODO(hjd): Transitional until we send the full PMD. See crbug.com/704203
  result->malloc_total_kb = GetDumpsSumKb("malloc", process_memory_dump);
  result->v8_total_kb = GetDumpsSumKb("v8/*", process_memory_dump);

  result->command_buffer_total_kb =
      GetDumpsSumKb("gpu/gl/textures/*", process_memory_dump);
  result->command_buffer_total_kb +=
      GetDumpsSumKb("gpu/gl/buffers/*", process_memory_dump);
  result->command_buffer_total_kb +=
      GetDumpsSumKb("gpu/gl/renderbuffers/*", process_memory_dump);

  // partition_alloc reports sizes for both allocated_objects and
  // partitions. The memory allocated_objects uses is a subset of
  // the partitions memory so to avoid double counting we only
  // count partitions memory.
  result->partition_alloc_total_kb =
      GetDumpsSumKb("partition_alloc/partitions/*", process_memory_dump);
  result->blink_gc_total_kb = GetDumpsSumKb("blink_gc", process_memory_dump);
  return result;
}

}  // namespace


// static
CoordinatorImpl* CoordinatorImpl::GetInstance() {
  return g_coordinator_impl;
}

CoordinatorImpl::CoordinatorImpl(service_manager::Connector* connector)
    : next_dump_id_(0) {
  process_map_ = base::MakeUnique<ProcessMap>(connector);
  DCHECK(!g_coordinator_impl);
  g_coordinator_impl = this;
  base::trace_event::MemoryDumpManager::GetInstance()->set_tracing_process_id(
      mojom::kServiceTracingProcessId);

  tracing_observer_ = base::MakeUnique<TracingObserver>(
      base::trace_event::TraceLog::GetInstance(), nullptr);
}

CoordinatorImpl::~CoordinatorImpl() {
  g_coordinator_impl = nullptr;
}

base::ProcessId CoordinatorImpl::GetProcessIdForClientIdentity(
    service_manager::Identity identity) const {
  // TODO(primiano): the browser process registers bypassing mojo and ends up
  // with an invalid identity. Fix that (crbug.com/733165) and remove the
  // special case in the else branch below.
  if (!identity.IsValid()) {
    return base::GetCurrentProcId();
  }
  return process_map_->GetProcessId(identity);
}

service_manager::Identity CoordinatorImpl::GetClientIdentityForCurrentRequest()
    const {
  return bindings_.dispatch_context();
}

void CoordinatorImpl::BindCoordinatorRequest(
    mojom::CoordinatorRequest request,
    const service_manager::BindSourceInfo& source_info) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  bindings_.AddBinding(this, std::move(request), source_info.identity);
}

void CoordinatorImpl::RequestGlobalMemoryDump(
    const base::trace_event::MemoryDumpRequestArgs& args_in,
    const RequestGlobalMemoryDumpCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  UMA_HISTOGRAM_COUNTS_1000("Memory.Experimental.Debug.GlobalDumpQueueLength",
                            queued_memory_dump_requests_.size());

  bool another_dump_already_in_progress = !queued_memory_dump_requests_.empty();

  // TODO(primiano): remove dump_guid from the request. For the moment callers
  // should just pass a zero |dump_guid| in input. It should be an out-only arg.
  DCHECK_EQ(0u, args_in.dump_guid);
  base::trace_event::MemoryDumpRequestArgs args = args_in;
  args.dump_guid = ++next_dump_id_;

  // If this is a periodic or peak memory dump request and there already is
  // another request in the queue with the same level of detail, there's no
  // point in enqueuing this request.
  if (another_dump_already_in_progress &&
      args.dump_type !=
          base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED &&
      args.dump_type != base::trace_event::MemoryDumpType::SUMMARY_ONLY &&
      args.dump_type != base::trace_event::MemoryDumpType::VM_REGIONS_ONLY) {
    for (const auto& request : queued_memory_dump_requests_) {
      if (request.args.level_of_detail == args.level_of_detail) {
        VLOG(1) << "RequestGlobalMemoryDump("
                << base::trace_event::MemoryDumpTypeToString(args.dump_type)
                << ") skipped because another dump request with the same "
                   "level of detail ("
                << base::trace_event::MemoryDumpLevelOfDetailToString(
                       args.level_of_detail)
                << ") is already in the queue";
        callback.Run(false /* success */, args.dump_guid,
                     nullptr /* global_memory_dump */);
        return;
      }
    }
  }

  queued_memory_dump_requests_.emplace_back(args, callback);

  // If another dump is already in progress, this dump will automatically be
  // scheduled when the other dump finishes.
  if (another_dump_already_in_progress)
    return;

  PerformNextQueuedGlobalMemoryDump();
}

void CoordinatorImpl::GetVmRegionsForHeapProfiler(
    const GetVmRegionsForHeapProfilerCallback& callback) {
  base::trace_event::MemoryDumpRequestArgs args{
      0 /* dump_guid */, base::trace_event::MemoryDumpType::VM_REGIONS_ONLY,
      base::trace_event::MemoryDumpLevelOfDetail::DETAILED};

  // This merely strips out the |dump_guid| argument, which is not used by
  // the GetVmRegionsForHeapProfiler() callback.
  auto callback_adapter =
      [](const GetVmRegionsForHeapProfilerCallback& vm_regions_callback,
         bool success, uint64_t,
         mojom::GlobalMemoryDumpPtr global_memory_dump) {
        vm_regions_callback.Run(success, std::move(global_memory_dump));
      };
  RequestGlobalMemoryDump(args, base::Bind(callback_adapter, callback));
}

void CoordinatorImpl::RegisterClientProcess(
    mojom::ClientProcessPtr client_process_ptr,
    mojom::ProcessType process_type) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::ClientProcess* client_process = client_process_ptr.get();
  client_process_ptr.set_connection_error_handler(
      base::Bind(&CoordinatorImpl::UnregisterClientProcess,
                 base::Unretained(this), client_process));
  auto client_info =
      base::MakeUnique<ClientInfo>(GetClientIdentityForCurrentRequest(),
                                   std::move(client_process_ptr), process_type);
  auto iterator_and_inserted =
      clients_.emplace(client_process, std::move(client_info));
  DCHECK(iterator_and_inserted.second);
}

void CoordinatorImpl::UnregisterClientProcess(
    mojom::ClientProcess* client_process) {
  QueuedMemoryDumpRequest* request = GetCurrentRequest();
  if (request != nullptr) {
    // Check if we are waiting for an ack from this client process.
    auto it = request->pending_responses.begin();
    while (it != request->pending_responses.end()) {
      // The calls to On*MemoryDumpResponse below, if executed, will delete the
      // element under the iterator which invalidates it. To avoid this we
      // increment the iterator in advance while keeping a reference to the
      // current element.
      std::set<QueuedMemoryDumpRequest::PendingResponse>::iterator current =
          it++;
      if (current->client != client_process)
        continue;
      RemovePendingResponse(client_process, current->type);
      request->failed_memory_dump_count++;
      // Regression test (crbug.com/742265).
      DCHECK(GetCurrentRequest());
    }
    FinalizeGlobalMemoryDumpIfAllManagersReplied();
  }
  size_t num_deleted = clients_.erase(client_process);
  DCHECK(num_deleted == 1);
}

void CoordinatorImpl::PerformNextQueuedGlobalMemoryDump() {
  using ResponseType = QueuedMemoryDumpRequest::PendingResponse::Type;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedMemoryDumpRequest* request = GetCurrentRequest();
  if (request == nullptr) {
    NOTREACHED() << "No current dump request.";
    return;
  }

  const bool wants_mmaps =
      request->args.level_of_detail == MemoryDumpLevelOfDetail::DETAILED;
  const bool dump_only_vm_regions =
      request->args.dump_type == MemoryDumpType::VM_REGIONS_ONLY;
  // VM_REGIONS_ONLY dumps must have |level_of_detail| == DETAILED.
  DCHECK(!dump_only_vm_regions || wants_mmaps);

  request->start_time = base::Time::Now();

  TRACE_EVENT_NESTABLE_ASYNC_BEGIN2(
      base::trace_event::MemoryDumpManager::kTraceCategory, "GlobalMemoryDump",
      TRACE_ID_LOCAL(request->args.dump_guid), "dump_type",
      base::trace_event::MemoryDumpTypeToString(request->args.dump_type),
      "level_of_detail",
      base::trace_event::MemoryDumpLevelOfDetailToString(
          request->args.level_of_detail));

  request->failed_memory_dump_count = 0;

  // Note: the service process itself is registered as a ClientProcess and
  // will be treated like any other process for the sake of memory dumps.
  request->pending_responses.clear();

  for (const auto& kv : clients_) {
    mojom::ClientProcess* client = kv.second->client.get();
    service_manager::Identity client_identity = kv.second->identity;
    const base::ProcessId pid = GetProcessIdForClientIdentity(client_identity);
    if (pid == base::kNullProcessId) {
      // TODO(hjd): Change to NOTREACHED when crbug.com/739710 is fixed.
      VLOG(1) << "Couldn't find a PID for client \"" << client_identity.name()
              << "." << client_identity.instance() << "\"";
      continue;
    }
    request->responses[client].process_id = pid;
    request->responses[client].process_type = kv.second->process_type;

    // Don't request a chrome memory dump at all if the client wants only the
    // processes' vm regions, which are retrieved via RequestOSMemoryDump().
    if (request->WantsChromeDumps()) {
      request->pending_responses.insert({client, ResponseType::kChromeDump});
      auto callback = base::Bind(&CoordinatorImpl::OnChromeMemoryDumpResponse,
                                 base::Unretained(this), client);
      client->RequestChromeMemoryDump(request->args, callback);
    }

// On most platforms each process can dump data about their own process
// so ask each process to do so Linux is special see below.
#if !defined(OS_LINUX)
    request->pending_responses.insert({client, ResponseType::kOSDump});
    auto os_callback = base::Bind(&CoordinatorImpl::OnOSMemoryDumpResponse,
                                  base::Unretained(this), client);
    client->RequestOSMemoryDump(request->WantsMmaps(), {base::kNullProcessId},
                                os_callback);
#endif  // !defined(OS_LINUX)
  }

// In some cases, OS stats can only be dumped from a privileged process to
// get around to sandboxing/selinux restrictions (see crbug.com/461788).
#if defined(OS_LINUX)
  std::vector<base::ProcessId> pids;
  mojom::ClientProcess* browser_client = nullptr;
  pids.reserve(clients_.size());
  for (const auto& kv : clients_) {
    service_manager::Identity client_identity = kv.second->identity;
    const base::ProcessId pid = GetProcessIdForClientIdentity(client_identity);
    if (pid == base::kNullProcessId) {
      // TODO(hjd): Change to NOTREACHED when crbug.com/739710 is fixed.
      VLOG(1) << "Couldn't find a PID for client \"" << client_identity.name()
              << "." << client_identity.instance() << "\"";
      continue;
    }
    pids.push_back(pid);
    if (kv.second->process_type == mojom::ProcessType::BROWSER) {
      browser_client = kv.first;
    }
  }

  if (clients_.size() > 0) {
    DCHECK(browser_client);
  }
  if (browser_client) {
    request->pending_responses.insert({browser_client, ResponseType::kOSDump});
    const auto callback = base::Bind(&CoordinatorImpl::OnOSMemoryDumpResponse,
                                     base::Unretained(this), browser_client);
    browser_client->RequestOSMemoryDump(request->WantsMmaps(), pids, callback);
  }
#endif  // defined(OS_LINUX)

  // Run the callback in case there are no client processes registered.
  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

CoordinatorImpl::QueuedMemoryDumpRequest* CoordinatorImpl::GetCurrentRequest() {
  if (queued_memory_dump_requests_.empty()) {
    return nullptr;
  }
  return &queued_memory_dump_requests_.front();
}

void CoordinatorImpl::OnChromeMemoryDumpResponse(
    mojom::ClientProcess* client,
    bool success,
    uint64_t dump_guid,
    base::Optional<base::trace_event::ProcessMemoryDump> chrome_memory_dump) {
  using ResponseType = QueuedMemoryDumpRequest::PendingResponse::Type;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedMemoryDumpRequest* request = GetCurrentRequest();
  if (request == nullptr) {
    NOTREACHED() << "No current dump request.";
    return;
  }

  RemovePendingResponse(client, ResponseType::kChromeDump);

  if (!clients_.count(client)) {
    VLOG(1) << "Received a memory dump response from an unregistered client";
    return;
  }

  request->responses[client].chrome_dump = std::move(chrome_memory_dump);

  if (!success) {
    request->failed_memory_dump_count++;
    VLOG(1) << "RequestGlobalMemoryDump() FAIL: NACK from client process";
  }

  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

void CoordinatorImpl::OnOSMemoryDumpResponse(mojom::ClientProcess* client,
                                             bool success,
                                             OSMemDumpMap os_dumps) {
  using ResponseType = QueuedMemoryDumpRequest::PendingResponse::Type;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedMemoryDumpRequest* request = GetCurrentRequest();
  if (request == nullptr) {
    NOTREACHED() << "No current dump request.";
    return;
  }

  RemovePendingResponse(client, ResponseType::kOSDump);

  if (!clients_.count(client)) {
    VLOG(1) << "Received a memory dump response from an unregistered client";
    return;
  }

  request->responses[client].os_dumps = std::move(os_dumps);

  if (!success) {
    request->failed_memory_dump_count++;
    VLOG(1) << "RequestGlobalMemoryDump() FAIL: NACK from client process";
  }

  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

void CoordinatorImpl::RemovePendingResponse(
    mojom::ClientProcess* client,
    QueuedMemoryDumpRequest::PendingResponse::Type type) {
  QueuedMemoryDumpRequest* request = GetCurrentRequest();
  if (request == nullptr) {
    NOTREACHED() << "No current dump request.";
    return;
  }
  auto it = request->pending_responses.find({client, type});
  if (it == request->pending_responses.end()) {
    VLOG(1) << "Unexpected memory dump response";
    return;
  }
  request->pending_responses.erase(it);
}

struct ResultsForProcess {
  base::ProcessId process_id;
  mojom::ProcessType process_type;
  const base::trace_event::ProcessMemoryDump* raw_process_dump = nullptr;
  mojom::RawOSMemDump* raw_os_dump = nullptr;
};

void CoordinatorImpl::FinalizeGlobalMemoryDumpIfAllManagersReplied() {
  DCHECK(!queued_memory_dump_requests_.empty());
  QueuedMemoryDumpRequest* request = &queued_memory_dump_requests_.front();
  if (request->pending_responses.size() > 0)
    return;

  // Reconstruct a map of pid -> ProcessMemoryDump by reassembling the responses
  // received by the clients for this dump. In some cases the response coming
  // from one client can also provide the dump of OS counters for other
  // processes. A concrete case is Linux, where the browser process provides
  // details for the child processes to get around sandbox restrictions on
  // opening /proc pseudo files.

  std::map<base::ProcessId, ResultsForProcess> pid_to_results;
  for (auto& response : request->responses) {
    const base::ProcessId& original_pid = response.second.process_id;
    pid_to_results[original_pid].process_id = original_pid;
    pid_to_results[original_pid].process_type = response.second.process_type;

    const base::Optional<base::trace_event::ProcessMemoryDump>&
        raw_process_dump = response.second.chrome_dump;
    if (raw_process_dump.has_value())
      pid_to_results[original_pid].raw_process_dump = &raw_process_dump.value();

    // |response| accumulates the replies received by each client process.
    // Depending on the OS each client process might return 1 chrome + 1 OS
    // dump each or, in the case of Linux, only 1 chrome dump each % the
    // browser process which will provide all the OS dumps.
    // In the former case (!OS_LINUX) we expect client processes to have
    // exactly one OS dump in their |response|, % the case when they
    // unexpectedly disconnect in the middle of a dump (e.g. because they
    // crash). In the latter case (OS_LINUX) we expect the full map to come
    // from the browser process response.
    OSMemDumpMap& extra_os_dumps = response.second.os_dumps;
#if defined(OS_LINUX)
    for (auto& kv : extra_os_dumps) {
      base::ProcessId pid = kv.first;
      const mojom::RawOSMemDumpPtr& dump = kv.second;
      if (pid == base::kNullProcessId)
        pid = original_pid;
      DCHECK_EQ(pid_to_results[pid].raw_os_dump, nullptr);
      pid_to_results[pid].raw_os_dump = dump.get();
    }
#else
    // This can be empty if the client disconnects before providing both
    // dumps. See UnregisterClientProcess().
    const base::ProcessId pid = response.second.process_id;
    DCHECK_LE(extra_os_dumps.size(), 1u);
    if (extra_os_dumps.size() == 1u) {
      DCHECK_EQ(base::kNullProcessId, extra_os_dumps.begin()->first);
      DCHECK_EQ(pid_to_results[pid].raw_os_dump, nullptr);
      pid_to_results[pid].raw_os_dump = extra_os_dumps.begin()->second.get();
    }
#endif
  }

  // Discard results with missing data.
  std::vector<ResultsForProcess> all_results;
  all_results.reserve(pid_to_results.size());
  for (const auto& kv : pid_to_results) {
    const ResultsForProcess& result = kv.second;
    bool has_raw_proccess_dump = result.raw_process_dump;
    bool has_raw_os_dump = result.raw_os_dump;
    bool has_vm_regions =
        has_raw_os_dump && result.raw_os_dump->memory_maps.size() > 0;

    // If we have the OS dump we should have the platform private footprint.
    if (has_raw_os_dump)
      DCHECK(result.raw_os_dump->platform_private_footprint);

    bool valid = true;
    valid = valid && has_raw_os_dump;
    valid = valid && (!request->WantsChromeDumps() || has_raw_proccess_dump);
    valid = valid && (!request->WantsMmaps() || has_vm_regions);
    if (valid)
      all_results.push_back(result);
  }

  mojom::GlobalMemoryDumpPtr global_dump(mojom::GlobalMemoryDump::New());
  if (request->ShouldReturnSummaries() || request->ShouldReturnMmaps())
    global_dump->process_dumps.reserve(all_results.size());
  for (const ResultsForProcess& result : all_results) {
    mojom::OSMemDumpPtr os_dump = CreatePublicOSDump(*result.raw_os_dump);

    tracing_observer_->AddOsDumpToTraceIfEnabled(
        request->args, result.process_id, os_dump.get(),
        &result.raw_os_dump->memory_maps);

    if (!request->ShouldReturnMmaps() && !request->ShouldReturnSummaries())
      continue;

    if (request->ShouldReturnMmaps()) {
      os_dump->memory_maps_for_heap_profiler =
          std::move(result.raw_os_dump->memory_maps);
    }

    mojom::ChromeMemDumpPtr chrome_dump = nullptr;
    if (request->ShouldReturnSummaries()) {
      chrome_dump = CreateDumpSummary(*result.raw_process_dump);
    } else {
      chrome_dump = mojom::ChromeMemDump::New();
    }

    mojom::ProcessMemoryDumpPtr pmd = mojom::ProcessMemoryDump::New();
    pmd->pid = result.process_id;
    pmd->process_type = result.process_type;
    pmd->os_dump = std::move(os_dump);
    pmd->chrome_dump = std::move(chrome_dump);
    global_dump->process_dumps.push_back(std::move(pmd));
  }

  const auto& callback = request->callback;
  const bool global_success = request->failed_memory_dump_count == 0;
  callback.Run(global_success, request->args.dump_guid, std::move(global_dump));
  UMA_HISTOGRAM_MEDIUM_TIMES("Memory.Experimental.Debug.GlobalDumpDuration",
                             base::Time::Now() - request->start_time);
  UMA_HISTOGRAM_COUNTS_1000(
      "Memory.Experimental.Debug.FailedProcessDumpsPerGlobalDump",
      request->failed_memory_dump_count);

  char guid_str[20];
  sprintf(guid_str, "0x%" PRIx64, request->args.dump_guid);
  TRACE_EVENT_NESTABLE_ASYNC_END2(
      base::trace_event::MemoryDumpManager::kTraceCategory, "GlobalMemoryDump",
      TRACE_ID_LOCAL(request->args.dump_guid), "dump_guid",
      TRACE_STR_COPY(guid_str), "success", global_success);

  queued_memory_dump_requests_.pop_front();
  request = nullptr;

  // Schedule the next queued dump (if applicable).
  if (!queued_memory_dump_requests_.empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&CoordinatorImpl::PerformNextQueuedGlobalMemoryDump,
                   base::Unretained(this)));
  }
}

CoordinatorImpl::QueuedMemoryDumpRequest::Response::Response() {}
CoordinatorImpl::QueuedMemoryDumpRequest::Response::~Response() {}

CoordinatorImpl::QueuedMemoryDumpRequest::QueuedMemoryDumpRequest(
    const base::trace_event::MemoryDumpRequestArgs& args,
    const RequestGlobalMemoryDumpCallback callback)
    : args(args), callback(callback) {}

CoordinatorImpl::QueuedMemoryDumpRequest::~QueuedMemoryDumpRequest() {}

CoordinatorImpl::ClientInfo::ClientInfo(
    const service_manager::Identity& identity,
    mojom::ClientProcessPtr client,
    mojom::ProcessType process_type)
    : identity(identity),
      client(std::move(client)),
      process_type(process_type) {}
CoordinatorImpl::ClientInfo::~ClientInfo() {}

CoordinatorImpl::QueuedMemoryDumpRequest::PendingResponse::PendingResponse(
    const mojom::ClientProcess* client,
    Type type)
    : client(client), type(type) {}

bool CoordinatorImpl::QueuedMemoryDumpRequest::PendingResponse::operator<(
    const PendingResponse& other) const {
  return std::tie(client, type) < std::tie(other.client, other.type);
}

bool CoordinatorImpl::QueuedMemoryDumpRequest::WantsMmaps() const {
  return args.level_of_detail == MemoryDumpLevelOfDetail::DETAILED;
}

bool CoordinatorImpl::QueuedMemoryDumpRequest::WantsChromeDumps() const {
  return args.dump_type != MemoryDumpType::VM_REGIONS_ONLY;
}

bool CoordinatorImpl::QueuedMemoryDumpRequest::ShouldReturnMmaps() const {
  return args.dump_type == MemoryDumpType::VM_REGIONS_ONLY;
}

bool CoordinatorImpl::QueuedMemoryDumpRequest::ShouldReturnSummaries() const {
  return args.dump_type == MemoryDumpType::SUMMARY_ONLY;
}

}  // namespace memory_instrumentation
