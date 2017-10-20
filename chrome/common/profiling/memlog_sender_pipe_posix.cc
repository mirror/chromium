// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender_pipe_posix.h"

#include <poll.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"

namespace profiling {

MemlogSenderPipe::MemlogSenderPipe(base::ScopedPlatformFile file)
    : handle_(mojo::edk::PlatformHandle(file.release())) {}

MemlogSenderPipe::~MemlogSenderPipe() {
}

MemlogSenderPipe::Result MemlogSenderPipe::Send(const void* data, size_t sz) {
  base::AutoLock lock(lock_);

  // This can happen if Close() was called on another thread, while this thread
  // was already waiting to call MemlogSenderPipe::Send().
  if (!handle_.is_valid())
    return Result::kError;

  int size = static_cast<int>(sz);
  base::Time start_time;
  while (size > 0) {
    int r = mojo::edk::PlatformChannelWrite(handle_.get(), data, size);

    // On success!
    if (r != -1) {
      DCHECK_LE(r, size);
      size -= r;
      data = static_cast<const char*>(data) + r;
      continue;
    }

    // An error is either irrecoverable, or an I/O delay. Wait at most 10
    // seconds for the pipe to clear.
    int cached_errno = errno;
    if (cached_errno != EAGAIN && cached_errno != EWOULDBLOCK)
      return Result::kError;

    // Set the start time, if it hasn't already been set.
    base::Time now = base::Time::Now();
    if (start_time.is_null())
      start_time = now;

    // Calculate time left.
    int64_t timeout_ms = ((start_time + base::TimeDelta::FromSeconds(10)) - now)
                             .InMilliseconds();
    if (timeout_ms <= 0)
      return Result::kTimeout;

    // Wait for the pipe to be writeable.
    struct pollfd pfd = {handle_.get().handle, POLLOUT, 0};
    int poll_result = poll(&pfd, 1, static_cast<int>(timeout_ms));
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
