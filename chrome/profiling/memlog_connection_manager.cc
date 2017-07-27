// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_connection_manager.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread.h"
#include "chrome/profiling/allocation_tracker.h"
#include "chrome/profiling/memlog_receiver_pipe.h"
#include "chrome/profiling/memlog_stream_parser.h"

namespace profiling {

struct MemlogConnectionManager::Connection {
  Connection(AllocationTracker::CompleteCallback complete_cb,
             BacktraceStorage* backtrace_storage,
             int process_id,
             scoped_refptr<MemlogReceiverPipe> p)
      : thread(base::StringPrintf("Proc %d thread", process_id)),
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

MemlogConnectionManager::MemlogConnectionManager(base::TaskRunner* io_runner,
                                                 BacktraceStorage* backtrace_storage)
  : io_runner_(io_runner),
    backtrace_storage_(backtrace_storage) {
}

MemlogConnectionManager::~MemlogConnectionManager() {
}

void MemlogConnectionManager::OnNewConnection(
    scoped_refptr<MemlogReceiverPipe> new_pipe,
    int sender_pid) {
  // Task to post to clean up the connection. Don't need to retain |this| since
  // it wil be called by objects owned by the MemlogConnectionManager.
  AllocationTracker::CompleteCallback complete_cb =
      base::BindOnce(&MemlogConnectionManager::OnConnectionCompleteThunk,
                     base::Unretained(this),
                     base::MessageLoop::current()->task_runner(), sender_pid);

  std::unique_ptr<Connection> connection = base::MakeUnique<Connection>(
      std::move(complete_cb), backtrace_storage_, sender_pid, new_pipe);
  connection->thread.Start();

  connection->parser = new MemlogStreamParser(this, &connection->tracker);
  new_pipe->SetReceiver(connection->thread.task_runner(), connection->parser);

  connections_[sender_pid] = std::move(connection);

  io_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MemlogReceiverPipe::StartReadingOnIOThread, new_pipe));
}

void MemlogConnectionManager::OnConnectionComplete(int process_id) {
  auto found = connections_.find(process_id);
  CHECK(found != connections_.end());
  connections_.erase(found);
}

// Posts back to the given thread the connection complete message.
void MemlogConnectionManager::OnConnectionCompleteThunk(
    scoped_refptr<base::SingleThreadTaskRunner> main_loop,
    int process_id) {
  // This code is called by the allocation tracker which is owned by the
  // connection manager. When we tell the connection manager a connection is
  // done, we know the conncetion manager will still be in scope.
  main_loop->PostTask(FROM_HERE,
                      base::Bind(&MemlogConnectionManager::OnConnectionComplete,
                                 base::Unretained(this), process_id));
}

}  // namespace profiling
