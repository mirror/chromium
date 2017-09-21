// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/content_sandbox_type.h"

#include <string>

#include "content/public/common/content_switches.h"

namespace content {

void SetCommandLineFlagsForSandboxType(base::CommandLine* command_line,
                                       sandbox::SandboxType sandbox_type) {
  switch (sandbox_type) {
    case sandbox::SANDBOX_TYPE_NO_SANDBOX:
      command_line->AppendSwitch(switches::kNoSandbox);
      break;
    case sandbox::SANDBOX_TYPE_RENDERER:
      DCHECK(command_line->GetSwitchValueASCII(switches::kProcessType) ==
             switches::kRendererProcess);
      break;
    case sandbox::SANDBOX_TYPE_GPU:
      DCHECK(command_line->GetSwitchValueASCII(switches::kProcessType) ==
             switches::kGpuProcess);
      break;
    case sandbox::SANDBOX_TYPE_PPAPI:
      if (command_line->GetSwitchValueASCII(switches::kProcessType) !=
          switches::kPpapiPluginProcess) {
        DCHECK(command_line->GetSwitchValueASCII(switches::kProcessType) ==
               switches::kUtilityProcess);
        DCHECK(!command_line->HasSwitch(switches::kUtilityProcessSandboxType));
        command_line->AppendSwitchASCII(
            switches::kUtilityProcessSandboxType,
            sandbox::SandboxTypeToString(sandbox::SANDBOX_TYPE_PPAPI));
      }
      break;
    case sandbox::SANDBOX_TYPE_UTILITY:
    case sandbox::SANDBOX_TYPE_NETWORK:
    case sandbox::SANDBOX_TYPE_WIDEVINE:
      DCHECK(command_line->GetSwitchValueASCII(switches::kProcessType) ==
             switches::kUtilityProcess);
      DCHECK(!command_line->HasSwitch(switches::kUtilityProcessSandboxType));
      command_line->AppendSwitchASCII(
          switches::kUtilityProcessSandboxType,
          sandbox::SandboxTypeToString(sandbox_type));
      break;
    default:
      break;
  }
}

sandbox::SandboxType SandboxTypeFromCommandLine(
    const base::CommandLine& command_line) {
  if (command_line.HasSwitch(switches::kNoSandbox))
    return sandbox::SANDBOX_TYPE_NO_SANDBOX;

  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  if (process_type.empty())
    return sandbox::SANDBOX_TYPE_NO_SANDBOX;

  if (process_type == switches::kRendererProcess)
    return sandbox::SANDBOX_TYPE_RENDERER;

  if (process_type == switches::kGpuProcess) {
    if (command_line.HasSwitch(switches::kDisableGpuSandbox))
      return sandbox::SANDBOX_TYPE_NO_SANDBOX;
    return sandbox::SANDBOX_TYPE_GPU;
  }

  if (process_type == switches::kPpapiBrokerProcess)
    return sandbox::SANDBOX_TYPE_NO_SANDBOX;

  if (process_type == switches::kPpapiPluginProcess)
    return sandbox::SANDBOX_TYPE_PPAPI;

  if (process_type == switches::kUtilityProcess) {
    return sandbox::SandboxTypeFromString(
        command_line.GetSwitchValueASCII(switches::kUtilityProcessSandboxType));
  }

  // This is a process which we don't know about, i.e. an embedder-defined
  // process. If the embedder wants it sandboxed, they have a chance to return
  // the sandbox profile in ContentClient::GetSandboxProfileForSandboxType.
  return sandbox::SANDBOX_TYPE_INVALID;
}

}  // namespace content
