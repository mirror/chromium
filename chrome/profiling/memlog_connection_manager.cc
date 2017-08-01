// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_connection_manager.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread.h"
#include "chrome/profiling/allocation_tracker.h"
#include "chrome/profiling/json_exporter.h"
#include "chrome/profiling/memlog_receiver_pipe.h"
#include "chrome/profiling/memlog_stream_parser.h"

namespace profiling {

struct MemlogConnectionManager::Connection {
  Connection(AllocationTracker::CompleteCallback complete_cb,
             BacktraceStorage* backtrace_storage,
             int sender_id,
             scoped_refptr<MemlogReceiverPipe> p)
      : thread(base::StringPrintf("Sender %d thread", sender_id)),
        pipe(p),
        tracker(std::move(complete_cb), backtrace_storage) {}

  ~Connection() {
    // The parser may outlive this class because it's refcounted, make sure no
    // callbacks are issued.
    parser->DisconnectReceivers();
  }

  base::Thread thread;

  scoped_refptr<MemlogReceiverPipe> pipe;
  scoped_refptr<MemlogStreamParser> parser;
  AllocationTracker tracker;
};

MemlogConnectionManager::MemlogConnectionManager(
    scoped_refptr<base::SequencedTaskRunner> io_runner,
    BacktraceStorage* backtrace_storage)
    : io_runner_(io_runner), backtrace_storage_(backtrace_storage) {}

MemlogConnectionManager::~MemlogConnectionManager() {}

void MemlogConnectionManager::OnNewConnection(base::ScopedPlatformFile file,
                                              int sender_id) {
  LOG(ERROR) << "MemlogConnectionManager::OnNewConnection: " << sender_id;
  DCHECK(connections_.find(sender_id) == connections_.end());

  scoped_refptr<MemlogReceiverPipe> new_pipe =
      new MemlogReceiverPipe(std::move(file));
  // Task to post to clean up the connection. Don't need to retain |this| since
  // it wil be called by objects owned by the MemlogConnectionManager.
  AllocationTracker::CompleteCallback complete_cb =
      base::BindOnce(&MemlogConnectionManager::OnConnectionCompleteThunk,
                     base::Unretained(this),
                     base::MessageLoop::current()->task_runner(), sender_id);

  std::unique_ptr<Connection> connection = base::MakeUnique<Connection>(
      std::move(complete_cb), backtrace_storage_, sender_id, new_pipe);
  connection->thread.Start();

  connection->parser = new MemlogStreamParser(&connection->tracker);
  new_pipe->SetReceiver(connection->thread.task_runner(), connection->parser);

  connections_[sender_id] = std::move(connection);

  io_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MemlogReceiverPipe::StartReadingOnIOThread, new_pipe));
}

void MemlogConnectionManager::OnConnectionComplete(int sender_id) {
  auto found = connections_.find(sender_id);
  CHECK(found != connections_.end());
  found->second.release();
  connections_.erase(found);
}

// Posts back to the given thread the connection complete message.
void MemlogConnectionManager::OnConnectionCompleteThunk(
    scoped_refptr<base::SingleThreadTaskRunner> main_loop,
    int sender_id) {
  // This code is called by the allocation tracker which is owned by the
  // connection manager. When we tell the connection manager a connection is
  // done, we know the conncetion manager will still be in scope.
  main_loop->PostTask(FROM_HERE,
                      base::Bind(&MemlogConnectionManager::OnConnectionComplete,
                                 base::Unretained(this), sender_id));
}

void RunCallback(std::unique_ptr<std::ostringstream> result, profiling::mojom::Memlog::DumpProcessCallback callback) {
	std::move(callback).Run(result->str());
}

void MemlogConnectionManager::DumpProcess(int32_t sender_id, profiling::mojom::Memlog::DumpProcessCallback callback, scoped_refptr<base::SequencedTaskRunner> runner) {
  if (connections_.empty()) {
    LOG(ERROR) << "No connections found for process dumping";
    // Threading issues. Needs to be disaptched onto runner.
    std::move(callback).Run("");
    return;
  }

  LOG(ERROR) << "DumpProcess called for " << sender_id;
  std::vector<std::unique_ptr<base::AutoLock>> locks;
  for (auto& it : connections_) {
    Connection* connection = it.second.get();
    // Lock all connections to prevent deallocations of atoms from
    // BacktraceStorage. This only works if no new connections are made.
    locks.push_back(std::make_unique<base::AutoLock>(connection->parser->lock));
  }
  std::unique_ptr<std::ostringstream> oss(new std::ostringstream);
	int pid = 13;
  Connection* connection = connections_.begin()->second.get();

  ExportAllocationEventSetToJSON(pid,
      connection->tracker.live_allocs(),
      *oss);
  runner->PostTask(
      FROM_HERE,
      base::BindOnce(&RunCallback, std::move(oss), std::move(callback)));
}

}  // namespace profiling
