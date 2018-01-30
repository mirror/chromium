// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <atltrace.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/process/memory.h"
#include "base/win/process_startup_helper.h"
#include "base/win/scoped_winrt_initializer.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/install_static/product_install_details.h"
#include "components/crash/content/app/crash_switches.h"
#include "components/crash/content/app/run_as_crashpad_handler_win.h"
#include "content/public/common/content_switches.h"
#include "notification_helper/com_server_module.h"

extern "C" int WINAPI wWinMain(HINSTANCE instance,
                               HINSTANCE prev_instance,
                               wchar_t* command_line,
                               int show_command) {
  install_static::InitializeProductDetailsForPrimaryModule();

  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;

  base::CommandLine::Init(0, nullptr);

  std::string process_type =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kProcessType);

  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    return crash_reporter::RunAsCrashpadHandler(
        *base::CommandLine::ForCurrentProcess(), base::FilePath(),
        switches::kProcessType, switches::kUserDataDir);
  }

  // Make sure the process exits cleanly on unexpected errors.
  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
  base::win::RegisterInvalidParamHandler();
  base::win::SetupCRT(*base::CommandLine::ForCurrentProcess());

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  if (!winrt_initializer.Succeeded()) {
    ATL::AtlTrace("Failed initializing win RT\n");
    return -1;
  }

  // Run the out-of-proc COM server.
  notification_helper::ComServerModule com_server_module;
  com_server_module.Run();

  return 0;
}
