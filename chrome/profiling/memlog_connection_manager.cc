// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_connection_manager.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread.h"
#include "chrome/common/profiling/memlog_client.h"
#include "chrome/profiling/allocation_tracker.h"
#include "chrome/profiling/json_exporter.h"
#include "chrome/profiling/memlog_receiver_pipe.h"
#include "chrome/profiling/memlog_stream_parser.h"
#include "mojo/public/cpp/system/buffer.h"
#include "third_party/zlib/zlib.h"

#if defined(OS_WIN)
#include <io.h>
#endif

namespace profiling {

namespace {
const size_t kMinSizeThreshold = 16 * 1024;
const size_t kMinCountThreshold = 1024;
}  // namespace

struct MemlogConnectionManager::Connection {
  Connection(AllocationTracker::CompleteCallback complete_cb,
             BacktraceStorage* backtrace_storage,
             base::ProcessId pid,
             mojom::MemlogClientPtr client,
             scoped_refptr<MemlogReceiverPipe> p)
      : thread(base::StringPrintf("Sender %lld thread",
                                  static_cast<long long>(pid))),
        client(std::move(client)),
        pipe(p),
        tracker(std::move(complete_cb), backtrace_storage) {}

  ~Connection() {
    // The parser may outlive this class because it's refcounted, make sure no
    // callbacks are issued.
    parser->DisconnectReceivers();
  }

  base::Thread thread;

  mojom::MemlogClientPtr client;
  scoped_refptr<MemlogReceiverPipe> pipe;
  scoped_refptr<MemlogStreamParser> parser;

  // Danger: This lives on the |thread| member above. The connection manager
  // lives on the I/O thread, so accesses to the variable must be synchronized.
  AllocationTracker tracker;
};

MemlogConnectionManager::MemlogConnectionManager() : weak_factory_(this) {}
MemlogConnectionManager::~MemlogConnectionManager() = default;

void MemlogConnectionManager::OnNewConnection(
    base::ProcessId pid,
    mojom::MemlogClientPtr client,
    mojo::edk::ScopedPlatformHandle handle) {
  base::AutoLock lock(connections_lock_);
  DCHECK(connections_.find(pid) == connections_.end());

  scoped_refptr<MemlogReceiverPipe> new_pipe =
      new MemlogReceiverPipe(std::move(handle));

  // The allocation tracker will call this on a background thread, so thunk
  // back to the current thread with weak pointers.
  AllocationTracker::CompleteCallback complete_cb =
      base::BindOnce(&MemlogConnectionManager::OnConnectionCompleteThunk,
                     base::MessageLoop::current()->task_runner(),
                     weak_factory_.GetWeakPtr(), pid);

  std::unique_ptr<Connection> connection =
      base::MakeUnique<Connection>(std::move(complete_cb), &backtrace_storage_,
                                   pid, std::move(client), new_pipe);
  connection->thread.Start();

  connection->parser = new MemlogStreamParser(&connection->tracker);
  new_pipe->SetReceiver(connection->thread.task_runner(), connection->parser);

  connections_[pid] = std::move(connection);

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&MemlogReceiverPipe::StartReadingOnIOThread, new_pipe));
}

void MemlogConnectionManager::OnConnectionComplete(base::ProcessId pid) {
  base::AutoLock lock(connections_lock_);
  auto found = connections_.find(pid);
  CHECK(found != connections_.end());
  connections_.erase(found);
}

// static
void MemlogConnectionManager::OnConnectionCompleteThunk(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::WeakPtr<MemlogConnectionManager> connection_manager,
    base::ProcessId pid) {
  task_runner->PostTask(
      FROM_HERE, base::BindOnce(&MemlogConnectionManager::OnConnectionComplete,
                                connection_manager, pid));
}

void MemlogConnectionManager::DumpProcess(DumpProcessArgs args) {
  SynchronizeOnPid(args.pid,
                   base::BindOnce(&MemlogConnectionManager::DoDumpProcess,
                                  weak_factory_.GetWeakPtr(), std::move(args)));
}

void MemlogConnectionManager::DumpProcessForTracing(
    base::ProcessId pid,
    mojom::Memlog::DumpProcessForTracingCallback callback,
    std::vector<memory_instrumentation::mojom::VmRegionPtr> maps) {
  SynchronizeOnPid(
      pid, base::BindOnce(&MemlogConnectionManager::DoDumpProcessForTracing,
                          weak_factory_.GetWeakPtr(), pid, std::move(callback),
                          std::move(maps)));
}

void MemlogConnectionManager::SynchronizeOnPid(
    base::ProcessId process_id,
    AllocationTracker::SnapshotCallback callback) {
  base::AutoLock lock(connections_lock_);

  auto task_runner = base::MessageLoop::current()->task_runner();

  auto it = connections_.find(process_id);
  if (it == connections_.end()) {
    DLOG(ERROR) << "No connections found for memory dump for pid:"
                << process_id;
    std::move(callback).Run(false, AllocationCountMap(),
                            AllocationTracker::ContextMap());
    return;
  }

  int barrier_id = next_barrier_id_++;

  // Register for callback before requesting the dump so we don't race for the
  // signal. The callback will be issued on the allocation tracker thread so
  // need to thunk back to the I/O thread.
  Connection* connection = it->second.get();
  connection->tracker.SnapshotOnBarrier(barrier_id, task_runner,
                                        std::move(callback));
  connection->client->FlushPipe(barrier_id);
}

void MemlogConnectionManager::DoDumpProcess(
    DumpProcessArgs args,
    bool success,
    AllocationCountMap counts,
    AllocationTracker::ContextMap context) {
  if (!success) {
    std::move(args.callback).Run(false);
    return;
  }

  base::AutoLock lock(connections_lock_);

  // Lock all connections to prevent deallocations of atoms from
  // BacktraceStorage. This only works if no new connections are made, which
  // connections_lock_ guarantees.
  std::vector<std::unique_ptr<base::AutoLock>> locks;
  for (auto& it : connections_) {
    Connection* connection = it.second.get();
    locks.push_back(
        base::MakeUnique<base::AutoLock>(*connection->parser->GetLock()));
  }

  auto it = connections_.find(args.pid);
  if (it == connections_.end()) {
    DLOG(WARNING) << "Connection destroyed before dump could be taken.";
    std::move(args.callback).Run(false);
    return;
  }

  std::ostringstream oss;
  ExportParams params;
  params.allocs = std::move(counts);
  params.context_map = std::move(context);
  params.maps = &args.maps;
  params.min_size_threshold = kMinSizeThreshold;
  params.min_count_threshold = kMinCountThreshold;
  ExportAllocationEventSetToJSON(args.pid, params, std::move(args.metadata),
                                 oss);
  std::string reply = oss.str();

  // Pass ownership of the underlying fd/HANDLE to zlib.
  base::PlatformFile platform_file = args.file.TakePlatformFile();
#if defined(OS_WIN)
  // The underlying handle |platform_file| is also closed when |fd| is closed.
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(platform_file), 0);
#else
  int fd = platform_file;
#endif
  gzFile gz_file = gzdopen(fd, "w");
  if (!gz_file) {
    DLOG(ERROR) << "Cannot compress trace file";
    std::move(args.callback).Run(false);
    return;
  }

  size_t written_bytes = gzwrite(gz_file, reply.c_str(), reply.size());
  gzclose(gz_file);

  std::move(args.callback).Run(written_bytes == reply.size());
}

void MemlogConnectionManager::DoDumpProcessForTracing(
    base::ProcessId pid,
    mojom::Memlog::DumpProcessForTracingCallback callback,
    std::vector<memory_instrumentation::mojom::VmRegionPtr> maps,
    bool success,
    AllocationCountMap counts,
    AllocationTracker::ContextMap context) {
  if (!success) {
    std::move(callback).Run(mojo::ScopedSharedBufferHandle(), 0);
    return;
  }

  base::AutoLock lock(connections_lock_);

  // Lock all connections to prevent deallocations of atoms from
  // BacktraceStorage. This only works if no new connections are made, which
  // connections_lock_ guarantees.
  std::vector<std::unique_ptr<base::AutoLock>> locks;
  for (auto& it : connections_) {
    Connection* connection = it.second.get();
    locks.push_back(
        base::MakeUnique<base::AutoLock>(*connection->parser->GetLock()));
  }

  std::ostringstream oss;
  ExportParams params;
  params.allocs = std::move(counts);
  params.maps = &maps;
  params.context_map = std::move(context);
  params.min_size_threshold = kMinSizeThreshold;
  params.min_count_threshold = kMinCountThreshold;
  ExportMemoryMapsAndV2StackTraceToJSON(params, oss);
  std::string reply = oss.str();

  mojo::ScopedSharedBufferHandle buffer =
      mojo::SharedBufferHandle::Create(reply.size());
  if (!buffer.is_valid()) {
    DLOG(ERROR) << "Could not create Mojo shared buffer";
    std::move(callback).Run(std::move(buffer), 0);
    return;
  }

  mojo::ScopedSharedBufferMapping mapping = buffer->Map(reply.size());
  if (!mapping) {
    DLOG(ERROR) << "Could not map Mojo shared buffer";
    std::move(callback).Run(mojo::ScopedSharedBufferHandle(), 0);
    return;
  }

  memcpy(mapping.get(), reply.c_str(), reply.size());

  std::move(callback).Run(std::move(buffer), reply.size());
}

MemlogConnectionManager::DumpProcessArgs::DumpProcessArgs() = default;
MemlogConnectionManager::DumpProcessArgs::DumpProcessArgs(
    DumpProcessArgs&&) noexcept = default;
MemlogConnectionManager::DumpProcessArgs::~DumpProcessArgs() = default;

}  // namespace profiling
