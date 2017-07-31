// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender.h"

#include "base/command_line.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/constants.mojom.h"
#include "chrome/common/profiling/memlog.mojom.h"
#include "chrome/common/profiling/memlog_allocator_shim.h"
#include "chrome/common/profiling/memlog_sender_pipe.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

#if defined(OS_POSIX)
#include "base/posix/global_descriptors.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/profiling/profiling_constants.h"
#include "content/public/common/content_switches.h"
#endif

namespace profiling {

namespace {

// TODO(brettw) this is a hack to allow StartProfilingMojo to work. Figure out
// how to get the lifetime of this that allows that function call to work.
MemlogSenderPipe* memlog_sender_pipe = nullptr;

struct RequestContext {
  mojom::MemlogPtr memlog;
};

}  // namespace

void InitMemlogSenderIfNecessary(
    content::ServiceManagerConnection* connection) {
  const base::CommandLine& cmdline = *base::CommandLine::ForCurrentProcess();
  std::string pipe_id_str = cmdline.GetSwitchValueASCII(switches::kMemlogPipe);
  if (pipe_id_str.empty()) {
    return;
  }

  RequestContext* context = new RequestContext();
  connection->GetConnector()->BindInterface(profiling::mojom::kServiceName,
                                            &context->memlog);
  base::ProcessId pid = base::Process::Current().Pid();
  context->memlog->GetMemlogPipe(pid, base::Bind(&StartMemlogSender));
}

void StartMemlogSender(mojo::ScopedHandle handle) {
  base::PlatformFile fd;
  CHECK_EQ(mojo::UnwrapPlatformFile(std::move(handle), &fd), MOJO_RESULT_OK);
  base::ScopedPlatformFile scoped_fd(fd);
  LOG(ERROR) << "Starting with fd: " << fd;

  static MemlogSenderPipe pipe(std::move(scoped_fd));
  memlog_sender_pipe = &pipe;

  StreamHeader header;
  header.signature = kStreamSignature;
  pipe.Send(&header, sizeof(StreamHeader));

  InitAllocatorShim(&pipe);
}

void StartProfilingMojo() {
  static bool started_mojo = false;

  if (!started_mojo) {
    started_mojo = true;
    StartMojoControlPacket start_mojo_message;
    start_mojo_message.op = kStartMojoControlPacketType;
    memlog_sender_pipe->Send(&start_mojo_message, sizeof(start_mojo_message));
  }
}

}  // namespace profiling
