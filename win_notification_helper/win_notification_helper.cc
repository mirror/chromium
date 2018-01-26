// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <atltrace.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/scoped_winrt_initializer.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/install_static/product_install_details.h"
#include "components/crash/content/app/crash_switches.h"
#include "components/crash/content/app/run_as_crashpad_handler_win.h"
#include "content/public/common/content_switches.h"
#include "win_notification_helper/notification_activator.h"

extern "C" int WINAPI wWinMain(HINSTANCE instance,
                               HINSTANCE prev_instance,
                               wchar_t* command_line,
                               int show_command) {
  install_static::InitializeProductDetailsForPrimaryModule();

  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;

  base::CommandLine::Init(0, NULL);

  std::string process_type =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kProcessType);

  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    return crash_reporter::RunAsCrashpadHandler(
        *base::CommandLine::ForCurrentProcess(), base::FilePath(),
        switches::kProcessType, switches::kUserDataDir);
  }

  // Initialize COM.
  base::win::ScopedCOMInitializer com_initializer(
      base::win::ScopedCOMInitializer::kMTA);
  if (!com_initializer.Succeeded()) {
    ATL::AtlTrace("Failed initializing COM\n");
    return -1;
  }

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  if (!winrt_initializer.Succeeded()) {
    ATL::AtlTrace("Failed initializing win RT\n");
    return -1;
  }

  // Start the registration/unregistration process for the NotificationActivator
  // class object.
  ActivatorRegisterer activator_registerer;
  activator_registerer.Run();

  return 0;
}
