// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_PROFILING_MEMLOG_RECEIVER_PIPE_POSIX_H_
#define CHROME_PROFILING_MEMLOG_RECEIVER_PIPE_POSIX_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_pump_win.h"
#include "base/strings/string16.h"
#include "build/build_config.h"

namespace base {
class TaskRunner;
}

namespace profiling {

class MemlogStreamReceiver;

class MemlogReceiverPipe
    : public base::RefCountedThreadSafe<MemlogReceiverPipe> {
 public:
  class CompletionThunk {
    // TODO(ajwong): This isn't correct in POSIX. Use the objects necessary for
    // MessageLoopForIO::WatchFileDescriptor() here instead.
  };

  explicit MemlogReceiverPipe(std::unique_ptr<CompletionThunk> thunk);
  ~MemlogReceiverPipe();

  void StartReadingOnIOThread();

  int GetRemoteProcessID();
  void SetReceiver(scoped_refptr<base::TaskRunner> task_runner,
                   scoped_refptr<MemlogStreamReceiver> receiver);

 private:
  DISALLOW_COPY_AND_ASSIGN(MemlogReceiverPipe);
};

}  // namespace profiling

#endif  // CHROME_PROFILING_MEMLOG_RECEIVER_PIPE_POSIX_H_
