// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender_pipe.h"

#include <poll.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"

#if defined(OS_MACOSX)
#include "base/posix/eintr_wrapper.h"
#endif

namespace profiling {

MemlogSenderPipe::Pipes MemlogSenderPipe::CreatePipes() {
#if defined(OS_MACOSX
  // On macOS, we create a pipe() rather than a socketpair(). This causes
  // writes to be much more performant.
  // https://bugs.chromium.org/p/chromium/issues/detail?id=776435
  int fds[2];
  pipe(fds);
  PCHECK(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
  PCHECK(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);
  PCHECK(fcntl(fds[0], F_SETNOSIGPIPE, 1) == 0);
  PCHECK(fcntl(fds[1], F_SETNOSIGPIPE, 1) == 0);

  profiling_service_->AddProfilingClient(
      pid, std::move(client), mojo::WrapPlatformFile(fds[1]),
      mojo::WrapPlatformFile(fds[0]), process_type);
#else
  // Writes to the data_channel must be atomic to ensure that the profiling
  // process can demux the messages. We accomplish this by making writes
  // synchronous, and protecting the write() itself with a Lock.
  // On Windows, this calls CreateNamedPipeW with output and input buffer size
  // suggestions of 4096. Perhaps this should be increased to 65536.
  mojo::edk::PlatformChannelPair data_channel(false /* client_is_blocking */);

  // Passes the client_for_profiling directly to the profiling process.
  // The client process can not start sending data until the pipe is ready,
  // so talking to the client is done in the AddSender completion callback.
  //
}

#if defined(OS_MACOSX)
MemlogSenderPipe::MemlogSenderPipe(base::ScopedPlatformFile file)
    : file_(std::move(file)) {}
#else
MemlogSenderPipe::MemlogSenderPipe(base::ScopedPlatformFile file)
    : handle_(mojo::edk::PlatformHandle(file.release())) {}
#endif

MemlogSenderPipe::~MemlogSenderPipe() {
}

MemlogSenderPipe::Result MemlogSenderPipe::Send(const void* data,
                                                size_t sz,
                                                int timeout_ms) {
  base::AutoLock lock(lock_);

  // This can happen if Close() was called on another thread, while this thread
  // was already waiting to call MemlogSenderPipe::Send().
  if (!handle_.is_valid())
    return Result::kError;

  int size = static_cast<int>(sz);
  base::Time start_time;
  while (size > 0) {
#if defined(OS_MACOSX)
    int r = HANDLE_EINTR(write(handle_.get().handle, data, size));
#else
    int r = mojo::edk::PlatformChannelWrite(handle_.get(), data, size);
#endif

    // On success!
    if (r != -1) {
      DCHECK_LE(r, size);
      size -= r;
      data = static_cast<const char*>(data) + r;
      continue;
    }

    // An error is either irrecoverable, or an I/O delay. Wait at most
    // timeout_ms seconds for the pipe to clear.
    int cached_errno = errno;
    if (cached_errno != EAGAIN && cached_errno != EWOULDBLOCK)
      return Result::kError;

    // Set the start time, if it hasn't already been set.
    base::Time now = base::Time::Now();
    if (start_time.is_null())
      start_time = now;

    // Calculate time left.
    int64_t time_left_ms =
        ((start_time + base::TimeDelta::FromMilliseconds(timeout_ms)) - now)
            .InMilliseconds();
    if (time_left_ms <= 0)
      return Result::kTimeout;

    // Wait for the pipe to be writeable.
    struct pollfd pfd = {handle_.get().handle, POLLOUT, 0};
    int poll_result = poll(&pfd, 1, static_cast<int>(time_left_ms));
    if (poll_result == 0)
      return Result::kTimeout;
    if (poll_result == -1)
      return Result::kError;

    // If POLLOUT isn't returned, the pipe isn't writeable.
    DCHECK_EQ(poll_result, 1);
    if (!(pfd.revents & POLLOUT))
      return Result::kError;
  }
  return Result::kSuccess;
}

void MemlogSenderPipe::Close() {
  base::AutoLock lock(lock_);
  handle_.reset();
}

}  // namespace profiling
