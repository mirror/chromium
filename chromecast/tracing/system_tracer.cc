// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/tracing/system_tracer.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_libevent.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket.h"
#include "base/trace_event/trace_config.h"

namespace chromecast {
namespace {

const char kSocketPath[] = "/dev/socket/cast_system_trace";

constexpr size_t kBufferSize = 1UL << 16;

sockaddr_un FillAddress() {
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  static_assert(sizeof(kSocketPath) <= sizeof(addr.sun_path),
                "Address too long");
  strncpy(addr.sun_path, kSocketPath, sizeof(addr.sun_path) - 1);
  addr.sun_family = AF_UNIX;
  return addr;
}

base::ScopedFD CreateClientSocket() {
  base::ScopedFD socket_fd(
      socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
  if (!socket_fd.is_valid()) {
    PLOG(ERROR) << "socket";
    return base::ScopedFD();
  }

  struct sockaddr_un addr = FillAddress();

  if (connect(socket_fd.get(), reinterpret_cast<struct sockaddr*>(&addr),
              sizeof(addr))) {
    PLOG(ERROR) << "connect: " << kSocketPath;
    return base::ScopedFD();
  }

  return socket_fd;
}

}  // namespace

SystemTracer::SystemTracer()
    : connection_watcher_(FROM_HERE),
      trace_pipe_watcher_(FROM_HERE),
      buffer_(new char[kBufferSize]) {}

SystemTracer::~SystemTracer() {}

void SystemTracer::StartTracing(base::StringPiece categories,
                                StartTracingCallback callback) {
  start_tracing_callback_ = std::move(callback);
  if (state_ != State::INITIAL) {
    FailStartTracing();
    return;
  }

  connection_fd_ = CreateClientSocket();
  if (!connection_fd_.is_valid()) {
    FailStartTracing();
    return;
  }

  if (!base::UnixDomainSocket::SendMsg(connection_fd_.get(), categories.data(),
                                       categories.size(), std::vector<int>())) {
    PLOG(ERROR) << "sendmsg";
    FailStartTracing();
    return;
  }

  base::MessageLoopForIO::current()->WatchFileDescriptor(
      connection_fd_.get(), true /* persistent */,
      base::MessageLoopForIO::WATCH_READ, &connection_watcher_, this);
  state_ = State::STARTING;
}

void SystemTracer::StopTracing(const StopTracingCallback& callback) {
  stop_tracing_callback_ = callback;
  if (state_ != State::TRACING) {
    FailStopTracing();
    return;
  }

  char stop_tracing_message[] = {0};
  if (!base::UnixDomainSocket::SendMsg(
          connection_fd_.get(), stop_tracing_message,
          sizeof(stop_tracing_message), std::vector<int>())) {
    PLOG(ERROR) << "sendmsg";
    FailStopTracing();
    return;
  }

  base::MessageLoopForIO::current()->WatchFileDescriptor(
      trace_pipe_fd_.get(), true /* persistent */,
      base::MessageLoopForIO::WATCH_READ, &connection_watcher_, this);
  state_ = State::READING;
}

void SystemTracer::OnFileCanReadWithoutBlocking(int fd) {
  if (state_ == State::STARTING) {
    DCHECK_EQ(fd, connection_fd_.get());
    ReceiveStartAckAndTracePipe();
    return;
  }
  if (state_ == State::READING) {
    DCHECK_EQ(fd, trace_pipe_fd_.get());
    ReceiveTraceData();
    return;
  }

  NOTREACHED();
}

void SystemTracer::OnFileCanWriteWithoutBlocking(int fd) {
  NOTREACHED();
}

void SystemTracer::ReceiveStartAckAndTracePipe() {
  DCHECK_EQ(state_, State::STARTING);

  std::vector<base::ScopedFD> fds;
  ssize_t received = base::UnixDomainSocket::RecvMsg(
      connection_fd_.get(), buffer_.get(), kBufferSize, &fds);
  if (received == 0) {
    LOG(ERROR) << "EOF from server";
    FailStartTracing();
    return;
  }
  if (received < 0) {
    PLOG(ERROR) << "recvmsg";
    FailStartTracing();
    return;
  }
  if (fds.size() != 1) {
    LOG(ERROR) << "Start ack missing trace pipe";
    FailStartTracing();
    return;
  }

  trace_pipe_fd_ = std::move(fds[0]);
  connection_watcher_.StopWatchingFileDescriptor();
  std::move(start_tracing_callback_).Run(Status::OK);
  state_ = State::TRACING;
}

void SystemTracer::ReceiveTraceData() {
  DCHECK_EQ(state_, State::READING);

  for (;;) {
    ssize_t bytes =
        HANDLE_EINTR(read(trace_pipe_fd_.get(), buffer_.get(), kBufferSize));
    if (bytes < 0) {
      if (errno == EAGAIN)
        return;  // Wait for more data.
      PLOG(ERROR) << "read: trace";
      FailStopTracing();
      return;
    }

    if (bytes == 0) {
      FinishTracing();
      return;
    }

    trace_data_.append(buffer_.get(), bytes);

    static constexpr size_t kPartialTraceDataSize = 1UL << 20;  // 1 MiB
    if (trace_data_.size() > kPartialTraceDataSize)
      SendPartialTraceData();
  }
}

void SystemTracer::FailStartTracing() {
  std::move(start_tracing_callback_).Run(Status::FAIL);
  Cleanup();
}

void SystemTracer::FailStopTracing() {
  stop_tracing_callback_.Run(Status::FAIL, "");
  Cleanup();
}

void SystemTracer::SendPartialTraceData() {
  DCHECK_EQ(state_, State::READING);
  stop_tracing_callback_.Run(Status::KEEP_GOING, std::move(trace_data_));
  trace_data_ = "";
}

void SystemTracer::FinishTracing() {
  DCHECK_EQ(state_, State::READING);
  stop_tracing_callback_.Run(Status::OK, std::move(trace_data_));
  Cleanup();
}

void SystemTracer::Cleanup() {
  connection_watcher_.StopWatchingFileDescriptor();
  connection_fd_.reset();
  trace_pipe_watcher_.StopWatchingFileDescriptor();
  trace_pipe_fd_.reset();
  start_tracing_callback_.Reset();
  stop_tracing_callback_.Reset();
  state_ = State::FINISHED;
}

}  // namespace chromecast
