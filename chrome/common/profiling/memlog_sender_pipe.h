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

  // Matches the native buffer size on the pipe.
  // On Windows and Linux, the default pipe buffer size is 64 * 1024.
  // On macOS, the default pipe buffer size is 16 * 1024, but grows to 64 * 1024
  // for large writes.
  static constexpr size_t kPipeSize = 64 * 1024;

  // Returns a pair of newly created pipes. Must be called from a privileged
  // process. The sender-pipe is non-blocking and has a buffer size of
  // |kPipeSize|.
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
  base::ScopedPlatformFile file_;

  // All calls to Send() are wrapped in a Lock, since the size of the data might
  // be larger than the maximum atomic write size of a pipe on Posix [PIPE_BUF].
  // On Windows, ::WriteFile() is not thread-safe.
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(MemlogSenderPipe);
};

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_H_
