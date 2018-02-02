// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/process/memory.h"
#include "base/win/process_startup_helper.h"
#include "base/win/scoped_winrt_initializer.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/install_static/product_install_details.h"
#include "content/public/common/content_switches.h"
#include "notification_helper/com_server_module.h"
#include "notification_helper/trace_util.h"

extern "C" int WINAPI wWinMain(HINSTANCE instance,
                               HINSTANCE prev_instance,
                               wchar_t* command_line,
                               int show_command) {
  install_static::InitializeProductDetailsForPrimaryModule();

  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;

  // Make sure the process exits cleanly on unexpected errors.
  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
  base::win::RegisterInvalidParamHandler();
  base::win::SetupCRT(*base::CommandLine::ForCurrentProcess());

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  if (!winrt_initializer.Succeeded()) {
    notification_helper::Trace(L"Failed initializing winRT\n");
    return -1;
  }

  // Run the out-of-proc COM server.
  notification_helper::ComServerModule com_server_module;
  com_server_module.Run();

  return 0;
}
