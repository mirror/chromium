// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender_pipe_posix.h"

#include <unistd.h>

#include "base/compiler_specific.h"
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

    // An error is either irrecoverable, or an I/O delay. Wait at most 10
    // seconds for the pipe to clear.
    if (r == -1) {
      int cached_errno = errno;
      if (cached_errno != EAGAIN && cached_errno != EWOULDBLOCK)
        return Result::kError;

      if (start_time.is_null()) {
        start_time = base::Time::Now();
      } else {
        base::TimeDelta delta = base::Time::Now() - start_time;
        if (delta > base::TimeDelta::FromSeconds(10))
          return Result::kTimeout;
      }

      // Yield the thread.
      sched_yield();
      continue;
    }

    DCHECK_LE(r, size);
    size -= r;
    data = static_cast<const char*>(data) + r;
  }
  return Result::kSuccess;
}

void MemlogSenderPipe::Close() {
  base::AutoLock lock(lock_);
  handle_.reset();
}

}  // namespace profiling
