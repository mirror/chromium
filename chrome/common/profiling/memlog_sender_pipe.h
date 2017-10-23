// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_H_
#define CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_H_

#include "build/build_config.h"

#include "base/files/platform_file.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace profiling {

class MemlogSenderPipe {
 public:
  struct Pipes {
    Pipes();
    Pipes(Pipes&&);
    mojo::edk::ScopedPlatformHandle sender;
    mojo::edk::ScopedPlatformHandle receiver;
    DISALLOW_COPY_AND_ASSIGN(Pipes);
  };

  // Returns a pair of newly created pipes. Must be called from a privileged
  // process. The sender-pipe is non-blocking and has a buffer size of 64kb.
  static Pipes CreatePipes();

  explicit MemlogSenderPipe(base::ScopedPlatformFile file);
  ~MemlogSenderPipe();

  enum class Result { kSuccess, kTimeout, kError };

  // Attempts to atomically write all the |data| into the pipe. kError is
  // returned on failure, kTimeout after |timeout_ms| milliseconds.
  Result Send(const void* data, size_t sz, int timeout_ms);

  // Closes the underlying pipe.
  void Close();

 private:
#if defined(OS_WIN) || defined(OS_MACOSX)
  base::ScopedPlatformFile file_;
#else
  mojo::edk::ScopedPlatformHandle handle_;
#endif

  // All calls to Send() are wrapped in a Lock, since the size of the data might
  // be larger than the maximum atomic write size of a pipe on Posix [PIPE_BUF].
  // On Windows, ::WriteFile() is not thread-safe.
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(MemlogSenderPipe);
};

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_H_
