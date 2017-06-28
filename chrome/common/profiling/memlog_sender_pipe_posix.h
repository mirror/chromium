// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_POSIX_H_
#define CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_POSIX_H_

#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace profiling {

class MemlogSenderPipe {
 public:
  explicit MemlogSenderPipe(mojo::ScopedMessagePipeHandle control_pipe);
  ~MemlogSenderPipe();

  bool Send(const void* data, size_t sz);

 private:
  mojo::ScopedMessagePipeHandle control_pipe_;

  DISALLOW_COPY_AND_ASSIGN(MemlogSenderPipe);
};

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_MEMLOG_SENDER_PIPE_POSIX_H_
