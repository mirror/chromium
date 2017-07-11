// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/profiling_main.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/profiling_constants.h"
#include "chrome/profiling/profiling_globals.h"
#include "chrome/profiling/profiling_process.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/edk/embedder/transport_protocol.h"

// ERASEME
#if defined(OS_LINUX)
#include "base/strings/string_number_conversions.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include <unistd.h>
#endif

#if defined(OS_WIN)
#include "base/win/win_util.h"
#endif

namespace profiling {

int ProfilingMain(const base::CommandLine& cmdline) {
printf("ProfilingMain ==================\n");
#if defined(OS_WIN)
  ::MessageBoxA(NULL, "Profiling process", "Profiling process", NULL);
#endif
  ProfilingGlobals globals;

  mojo::edk::Init();
  mojo::edk::ScopedIPCSupport ipc_support(
      globals.GetIORunner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

/*
  {
    std::string endpoint_str = cmdline.GetSwitchValueASCII(mojo::edk::PlatformChannelPair::kMojoPlatformChannelHandleSwitch);
    int endpoint = 0;
    base::StringToInt(endpoint_str, &endpoint);
    printf("========== Got mojo endpoint = %d\n", endpoint);

    while (true) {
      char buf[4];
      errno = 0;
      ssize_t result = read(endpoint, buf, 4);
      printf("  ==========   Read result = %d, errno = %d\n", (int)result, errno);
      for (ssize_t i = 0; i < result; i++)
        printf("  ==========     %d\n", (int)buf[i]);
      sleep(3);
    }
  }
  */

  std::string pipe_id = cmdline.GetSwitchValueASCII(switches::kMemlogPipe);
  globals.GetMemlogConnectionManager()->StartConnections(pipe_id);

  globals.RunMainMessageLoop();
  printf("========== Profiling process exit\n");

#if defined(OS_WIN)
  base::win::SetShouldCrashOnProcessDetach(false);
#endif
  return 0;
}

}  // namespace profiling
