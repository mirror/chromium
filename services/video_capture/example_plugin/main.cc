// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"

static const base::FilePath::CharType kSocketPath[] =
    "/var/run/video_capture_plugin/plugin.sock";

void WaitForIncomingChannelOnSocket() {
  base::FilePath socket_path(kSocketPath);
  mojo::edk::ScopedPlatformHandle socket_fd = mojo::edk::CreateServerHandle(
      mojo::edk::NamedPlatformHandle(socket_path.value()));
  if (!socket_fd.is_valid()) {
    LOG(ERROR) << "Failed to create the socket file: " << kSocketPath;
    return;
  }

  // Run forever!
  base::RunLoop().Run();
}

int main(int argc, char** argv) {
  mojo::edk::Init();
  WaitForIncomingChannelOnSocket();

  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  // As long as this object is alive, all EDK API surface relevant to IPC
  // connections is usable and message pipes which span a process boundary will
  // continue to function.
  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  VLOG(1) << "Mojo initialized";

  // Run forever!
  base::RunLoop().Run();

  return 0;
}
