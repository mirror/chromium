// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_H_
#define CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_H_

#include "build/build_config.h"

#include "base/files/platform_file.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"

#if defined(OS_POSIX)
#include "mojo/edk/embedder/scoped_platform_handle.h"
#endif

namespace profiling {

class MemlogSenderPipe {
 public:
  explicit MemlogSenderPipe(base::ScopedPlatformFile file);
  ~MemlogSenderPipe();

  enum class Result { kSuccess, kTimeout, kError };

  // Attempts to atomically write all the |data| into the pipe. kError is
  // returned on failure, kTimeout after |timeout_ms| milliseconds.
  Result Send(const void* data, size_t sz, int timeout_ms);

  // Closes the underlying pipe.
  void Close();

 private:
#if defined(OS_POSIX)
  mojo::edk::ScopedPlatformHandle handle_;
#else
  base::ScopedPlatformFile file_;
#endif

  // All calls to Send() are wrapped in a Lock, since the size of the data might
  // be larger than the maximum atomic write size of a pipe on Posix [PIPE_BUF].
  // On Windows, ::WriteFile() is not thread-safe.
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(MemlogSenderPipe);
};

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_H_
