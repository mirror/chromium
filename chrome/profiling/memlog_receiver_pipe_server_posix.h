// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_PROFILING_MEMLOG_RECEIVER_PIPE_SERVER_POSIX_H_
#define CHROME_PROFILING_MEMLOG_RECEIVER_PIPE_SERVER_POSIX_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_pump_libevent.h"
#include "base/threading/thread.h"
#include "chrome/profiling/memlog_receiver_pipe_posix.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace base {
class TaskRunner;
}  // namespace base

namespace profiling {

class MemlogReceiverPipe;

// This class listens for new pipe connections and creates new
// MemlogReceiverPipe objects for each one.
class MemlogReceiverPipeServer
    : public base::RefCountedThreadSafe<MemlogReceiverPipeServer> {
 public:
  using NewConnectionCallback =
      base::RepeatingCallback<void(scoped_refptr<MemlogReceiverPipe>)>;

  // |io_runner| is the task runner for the I/O thread. When a new connection is
  // established, the |on_new_conn| callback is called with the pipe.
  MemlogReceiverPipeServer(base::TaskRunner* io_runner,
                           mojo::ScopedMessagePipeHandle control_pipe,
                           NewConnectionCallback on_new_conn);

  void set_on_new_connection(NewConnectionCallback on_new_connection) {
    on_new_connection_ = on_new_connection;
  }

  // Starts the server which opens the pipe and begins accepting connections.
  void Start();

 private:
  friend class base::RefCountedThreadSafe<MemlogReceiverPipeServer>;
  ~MemlogReceiverPipeServer();

  void StartWatchOnIO();
  void OnReadable(MojoResult result);

  scoped_refptr<base::TaskRunner> io_runner_;
  mojo::ScopedMessagePipeHandle control_pipe_;
  NewConnectionCallback on_new_connection_;
  std::unique_ptr<mojo::SimpleWatcher> watcher_;

  DISALLOW_COPY_AND_ASSIGN(MemlogReceiverPipeServer);
};

}  // namespace profiling

#endif  // CHROME_PROFILING_MEMLOG_RECEIVER_PIPE_SERVER_POSIX_H_
