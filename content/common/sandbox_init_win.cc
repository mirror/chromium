// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/sandbox_init.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "base/win/scoped_process_information.h"
#include "content/common/content_switches_internal.h"
#include "content/common/sandbox_win.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/sandbox_init.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_types.h"
#include "services/service_manager/sandbox/sandbox_type.h"
#include "services/service_manager/sandbox/switches.h"

namespace content {

// Updates the command line arguments with debug-related flags. If debug flags
// have been used with this process, they will be filtered and added to
// command_line as needed.
void ProcessDebugFlags(base::CommandLine* command_line) {
  const base::CommandLine& current_cmd_line =
      *base::CommandLine::ForCurrentProcess();
  std::string type = command_line->GetSwitchValueASCII(switches::kProcessType);
  if (current_cmd_line.HasSwitch(switches::kWaitForDebuggerChildren)) {
    // Look to pass-on the kWaitForDebugger flag.
    std::string value = current_cmd_line.GetSwitchValueASCII(
        switches::kWaitForDebuggerChildren);
    if (value.empty() || value == type)
      command_line->AppendSwitch(switches::kWaitForDebugger);

    command_line->AppendSwitchASCII(switches::kWaitForDebuggerChildren, value);
  }
}

sandbox::ResultCode StartSandboxedProcess(
    SandboxedProcessLauncherDelegate* delegate,
    base::CommandLine* cmd_line,
    const base::HandlesToInheritVector& handles_to_inherit,
    base::Process* process) {
  std::string type_str = cmd_line->GetSwitchValueASCII(switches::kProcessType);
  TRACE_EVENT1("startup", "StartProcessWithAccess", "type", type_str);

  ProcessDebugFlags(cmd_line);
  return StartSandboxedProcessInternal(
      cmd_line, type_str, delegate->GetSandboxType(),
      delegate->DisableDefaultPolicy(), handles_to_inherit,
      base::Bind(&SandboxedProcessLauncherDelegate::PreSpawnTarget,
                 base::Unretained(delegate)),
      base::Bind(&SandboxedProcessLauncherDelegate::PostSpawnTarget,
                 base::Unretained(delegate)),
      process);
}

}  // namespace content
