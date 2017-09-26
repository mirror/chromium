// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif  // defined(OS_WIN)

#include "base/command_line.h"
#include "base/test/launcher/test_launcher.h"
#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif  // defined(OS_WIN)
#include "chrome/test/base/chrome_test_launcher.h"
#include "chrome/test/base/chrome_test_suite.h"

#if defined(OS_WIN)
void EnableHighDPISupport() {
  // Declared inline as constants and loaded through DLLs (inspired by DLL
  // loading in chrome_exe_main_win.cc) as they won't be provided by
  // <shellscalingapi.h> when building this file. This is possibly because this
  // test suite is built as a command-line application and not a desktop
  // application.
  typedef enum PROCESS_DPI_AWARENESS {
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
  } PROCESS_DPI_AWARENESS;
  typedef HRESULT(WINAPI * SetProcessDpiAwarenessPtr)(PROCESS_DPI_AWARENESS);
  SetProcessDpiAwarenessPtr set_process_dpi_awareness_func =
      reinterpret_cast<SetProcessDpiAwarenessPtr>(GetProcAddress(
          GetModuleHandleA("user32.dll"), "SetProcessDpiAwarenessInternal"));
  if (!set_process_dpi_awareness_func)
    return;
  bool per_monitor_platform =
      base::win::GetVersion() >= base::win::VERSION_WIN10;
  set_process_dpi_awareness_func(per_monitor_platform
                                     ? PROCESS_PER_MONITOR_DPI_AWARE
                                     : PROCESS_SYSTEM_DPI_AWARE);
}
#endif  // defined(OS_WIN)

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  size_t parallel_jobs = base::NumParallelJobs();
  if (parallel_jobs == 0U) {
    return 1;
  } else if (parallel_jobs > 1U) {
    parallel_jobs /= 2U;
  }

#if defined(OS_WIN)
  EnableHighDPISupport();
#endif  // defined(OS_WIN)

  ChromeTestSuiteRunner runner;
  ChromeTestLauncherDelegate delegate(&runner);
  return LaunchChromeTests(parallel_jobs, &delegate, argc, argv);
}
