// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender.h"

#include "base/command_line.h"
#include "base/files/file.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/memlog_allocator_shim.h"
#include "chrome/common/profiling/memlog_sender_pipe.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "mojo/edk/embedder/platform_channel_pair.h"

namespace profiling {

void InitMemlogSenderIfNecessary(const base::CommandLine& cmdline) {
  std::string pipe_id = cmdline.GetSwitchValueASCII(switches::kMemlogPipe);
  if (!pipe_id.empty()) {
    base::ScopedPlatformHandle pipe(
        mojo::edk::PlatformChannelPair::
            PassClientHandleFromParentProcessFromString(pipe_id)
                .release()
                .handle);
    StartMemlogSender(std::move(pipe));
  }
}

void StartMemlogSender(base::ScopedPlatformHandle data_pipe) {
  // TODO(ajwong): DCHECK if this is called twice.
  static MemlogSenderPipe pipe(std::move(data_pipe));

  StreamHeader header;
  header.signature = kStreamSignature;

  pipe.Send(&header, sizeof(StreamHeader));

  InitAllocatorShim(&pipe);
}

}  // namespace profiling
