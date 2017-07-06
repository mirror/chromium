// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_connection_manager.h"

#include "base/bind.h"
#include "base/files/scoped_file.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread.h"
#include "chrome/profiling/allocation_tracker.h"
#include "chrome/profiling/memlog_stream_parser.h"
#include "chrome/profiling/profiling_globals.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace profiling {

struct MemlogConnectionManager::Connection {
  Connection(AllocationTracker::CompleteCallback complete_cb,
             int process_id,
             scoped_refptr<MemlogReceiverPipe> p)
      : thread(base::StringPrintf("Proc %d thread", process_id)),
        pipe(p),
        tracker(std::move(complete_cb)) {}

  ~Connection() {}

  base::Thread thread;

  scoped_refptr<MemlogReceiverPipe> pipe;
  scoped_refptr<MemlogStreamParser> parser;
  AllocationTracker tracker;
};

MemlogConnectionManager::MemlogConnectionManager(mojom::MemlogRequest request)
    : binding_(this) {
  ProfilingGlobals::Get()->GetIORunner()->PostTask(
      FROM_HERE, base::BindOnce(&MemlogConnectionManager::BindOnIO,
                                base::Unretained(this), std::move(request)));
}

MemlogConnectionManager::~MemlogConnectionManager() {
  // Clear the callback since the server is refcounted and may outlive us.
  server_->set_on_new_connection(
      base::RepeatingCallback<void(scoped_refptr<MemlogReceiverPipe>)>());
}

void MemlogConnectionManager::StartConnections(
    const std::string& initial_pipe_id) {
  server_ = new MemlogReceiverPipeServer(
      ProfilingGlobals::Get()->GetIORunner(), initial_pipe_id,
      base::BindRepeating(&MemlogConnectionManager::OnNewConnection,
                          base::Unretained(this)));
  server_->Start();
}

void MemlogConnectionManager::AddNewSender(mojo::ScopedHandle sender_pipe,
                                           int32_t sender_pid) {
  base::PlatformFile sender_file;
  MojoResult result =
      mojo::UnwrapPlatformFile(std::move(sender_pipe), &sender_file);
  CHECK_EQ(result, MOJO_RESULT_OK);
  scoped_refptr<MemlogReceiverPipe> new_pipe =
      new MemlogReceiverPipe(base::ScopedPlatformHandle(sender_file));

  LOG(ERROR) << "Profiling: New sender: " << sender_pid;
  // Task to post to clean up the connection. Don't need to retain |this| since
  // it wil be called by objects owned by the MemlogConnectionManager.
  AllocationTracker::CompleteCallback complete_cb =
      base::BindOnce(&MemlogConnectionManager::OnConnectionCompleteThunk,
                     base::Unretained(this),
                     base::MessageLoop::current()->task_runner(), sender_pid);

  std::unique_ptr<Connection> connection = base::MakeUnique<Connection>(
      std::move(complete_cb), sender_pid, new_pipe);
  connection->thread.Start();

  connection->parser = new MemlogStreamParser(&connection->tracker);
  new_pipe->SetReceiver(connection->thread.task_runner(), connection->parser);

  connections_[sender_pid] = std::move(connection);
}

void MemlogConnectionManager::BindOnIO(mojom::MemlogRequest request) {
  binding_.Bind(std::move(request));
}

void MemlogConnectionManager::OnNewConnection(
    scoped_refptr<MemlogReceiverPipe> new_pipe) {
  int remote_process = new_pipe->GetRemoteProcessID();

  // Task to post to clean up the connection. Don't need to retain |this| since
  // it wil be called by objects owned by the MemlogConnectionManager.
  AllocationTracker::CompleteCallback complete_cb = base::BindOnce(
      &MemlogConnectionManager::OnConnectionCompleteThunk,
      base::Unretained(this), base::MessageLoop::current()->task_runner(),
      remote_process);

  std::unique_ptr<Connection> connection = base::MakeUnique<Connection>(
      std::move(complete_cb), remote_process, new_pipe);
  connection->thread.Start();

  connection->parser = new MemlogStreamParser(&connection->tracker);
  new_pipe->SetReceiver(connection->thread.task_runner(), connection->parser);

  connections_[remote_process] = std::move(connection);
}

void MemlogConnectionManager::OnConnectionComplete(int process_id) {
  auto found = connections_.find(process_id);
  CHECK(found != connections_.end());
  connections_.erase(found);

  // When all connections are closed, exit.
  if (connections_.empty())
    ProfilingGlobals::Get()->QuitWhenIdle();
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
