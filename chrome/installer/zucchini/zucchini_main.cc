// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/process/memory.h"
#include "build/build_config.h"
#include "chrome/installer/zucchini/main_utils.h"
#include "chrome/installer/zucchini/zucchini.h"

#if defined(OS_WIN)
#include "base/win/process_startup_helper.h"
#endif  // defined(OS_WIN)

namespace {

void InitLogging() {
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  settings.log_file = nullptr;
  settings.lock_log = logging::DONT_LOCK_LOG_FILE;
  settings.delete_old = logging::APPEND_TO_OLD_LOG_FILE;
  bool logging_res = logging::InitLogging(settings);
  CHECK(logging_res);
}

void InitErroHandling(const base::CommandLine& command_line) {
  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
#if defined(OS_WIN)
  base::win::RegisterInvalidParamHandler();
  base::win::SetupCRT(command_line);
#endif  // defined(OS_WIN)
}

}  // namespace

int main(int argc, const char* argv[]) {
  base::AtExitManager exit_manager;
  base::AtExitManager::RegisterTask(base::Bind(ResourceUsageTracker::Finish));

  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  InitLogging();
  InitErroHandling(command_line);

  // Register and dispatch commands.
  CommandRegistry registry(&std::cerr);
  registry.RegisterAll();
  ResourceUsageTracker::Start();  // Skip overhead from above initialization.
  auto ret = registry.Run(command_line);
  if (!registry.DidRunCommand()) {
    // If command never got run, then skip resource usage display to reduce
    // visual noise for usage help text.
    ResourceUsageTracker::Cancel();
  }
  return ret;
}
