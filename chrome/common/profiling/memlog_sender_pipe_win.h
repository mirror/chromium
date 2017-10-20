// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_WIN_H_
#define CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_WIN_H_

#include <windows.h>

#include <string>

#include "base/files/platform_file.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/synchronization/lock.h"

namespace profiling {

class MemlogSenderPipe {
 public:
  explicit MemlogSenderPipe(base::ScopedPlatformFile file);
  ~MemlogSenderPipe();

  enum class Result { kSuccess, kTimeout, kError };

  // Attempts to atomically write all the |data| into the pipe. kError is
  // returned on failure, kTimeout after 10s of timeout.
  Result Send(const void* data, size_t sz);

  // Closes the underlying pipe.
  void Close();

 private:
  base::ScopedPlatformFile file_;
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(MemlogSenderPipe);
};

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_WIN_H_
