// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/sandbox_init.h"

#include "base/base_switches.h"
#include "base/debug/activity_tracker.h"
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
    if (value.empty() || value == type) {
      command_line->AppendSwitch(switches::kWaitForDebugger);
    }
    command_line->AppendSwitchASCII(switches::kWaitForDebuggerChildren, value);
  }
}

bool InitializeSandbox(service_manager::SandboxType sandbox_type,
                       sandbox::SandboxInterfaceInfo* sandbox_info) {
  sandbox::BrokerServices* broker_services = sandbox_info->broker_services;
  if (broker_services) {
    if (!InitBrokerServices(broker_services))
      return false;

    // IMPORTANT: This piece of code needs to run as early as possible in the
    // process because it will initialize the sandbox broker, which requires the
    // process to swap its window station. During this time all the UI will be
    // broken. This has to run before threads and windows are created.
    if (!service_manager::IsUnsandboxedSandboxType(sandbox_type)) {
      // Precreate the desktop and window station used by the renderers.
      scoped_refptr<sandbox::TargetPolicy> policy =
          broker_services->CreatePolicy();
      sandbox::ResultCode result = policy->CreateAlternateDesktop(true);
      CHECK(sandbox::SBOX_ERROR_FAILED_TO_SWITCH_BACK_WINSTATION != result);
    }
    return true;
  }

  return service_manager::IsUnsandboxedSandboxType(sandbox_type) ||
         InitTargetServices(sandbox_info->target_services);
}

sandbox::ResultCode StartSandboxedProcess(
    SandboxedProcessLauncherDelegate* delegate,
    base::CommandLine* cmd_line,
    const base::HandlesToInheritVector& handles_to_inherit,
    base::Process* process) {
  DCHECK(delegate);
  const base::CommandLine& browser_command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string type_str = cmd_line->GetSwitchValueASCII(switches::kProcessType);

  TRACE_EVENT1("startup", "StartProcessWithAccess", "type", type_str);

  // Propagate the --allow-no-job flag if present.
  if (browser_command_line.HasSwitch(
          service_manager::switches::kAllowNoSandboxJob) &&
      !cmd_line->HasSwitch(service_manager::switches::kAllowNoSandboxJob)) {
    cmd_line->AppendSwitch(service_manager::switches::kAllowNoSandboxJob);
  }

  ProcessDebugFlags(cmd_line);

  if (service_manager::IsUnsandboxedSandboxType(delegate->GetSandboxType()) ||
      browser_command_line.HasSwitch(switches::kNoSandbox) ||
      cmd_line->HasSwitch(switches::kNoSandbox)) {
    base::LaunchOptions options;
    options.handles_to_inherit = handles_to_inherit;
    *process = base::LaunchProcess(*cmd_line, options);
    return sandbox::SBOX_ALL_OK;
  }

  scoped_refptr<sandbox::TargetPolicy> policy =
      g_broker_services->CreatePolicy();

  // Add any handles to be inherited to the policy.
  for (HANDLE handle : handles_to_inherit)
    policy->AddHandleToShare(handle);

  // Pre-startup mitigations.
  sandbox::MitigationFlags mitigations =
      sandbox::MITIGATION_HEAP_TERMINATE | sandbox::MITIGATION_BOTTOM_UP_ASLR |
      sandbox::MITIGATION_DEP | sandbox::MITIGATION_DEP_NO_ATL_THUNK |
      sandbox::MITIGATION_EXTENSION_POINT_DISABLE | sandbox::MITIGATION_SEHOP |
      sandbox::MITIGATION_NONSYSTEM_FONT_DISABLE |
      sandbox::MITIGATION_IMAGE_LOAD_NO_REMOTE |
      sandbox::MITIGATION_IMAGE_LOAD_NO_LOW_LABEL;

  sandbox::ResultCode result = sandbox::SBOX_ERROR_GENERIC;
  result = policy->SetProcessMitigations(mitigations);

  if (result != sandbox::SBOX_ALL_OK)
    return result;

#if !defined(NACL_WIN64)
  if (type_str == switches::kRendererProcess &&
      service_manager::IsWin32kLockdownEnabled()) {
    result = AddWin32kLockdownPolicy(policy.get(), false);
    if (result != sandbox::SBOX_ALL_OK)
      return result;
  }
#endif

  // Post-startup mitigations.
  mitigations = sandbox::MITIGATION_STRICT_HANDLE_CHECKS |
                sandbox::MITIGATION_DLL_SEARCH_ORDER;
  if (base::FeatureList::IsEnabled(features::kWinSboxForceMsSigned))
    mitigations |= sandbox::MITIGATION_FORCE_MS_SIGNED_BINS;

  result = policy->SetDelayedProcessMitigations(mitigations);
  if (result != sandbox::SBOX_ALL_OK)
    return result;

  result = SetJobLevel(*cmd_line, sandbox::JOB_LOCKDOWN, 0, policy.get());
  if (result != sandbox::SBOX_ALL_OK)
    return result;

  if (!delegate->DisableDefaultPolicy()) {
    result = AddPolicyForSandboxedProcess(policy.get());
    if (result != sandbox::SBOX_ALL_OK)
      return result;
  }

#if !defined(NACL_WIN64)
  if (type_str == switches::kRendererProcess ||
      type_str == switches::kPpapiPluginProcess ||
      delegate->GetSandboxType() ==
          service_manager::SANDBOX_TYPE_PDF_COMPOSITOR) {
    AddDirectory(base::DIR_WINDOWS_FONTS, NULL, true,
                 sandbox::TargetPolicy::FILES_ALLOW_READONLY, policy.get());
  }
#endif

  if (type_str != switches::kRendererProcess) {
    // Hack for Google Desktop crash. Trick GD into not injecting its DLL into
    // this subprocess. See
    // http://code.google.com/p/chromium/issues/detail?id=25580
    cmd_line->AppendSwitchASCII("ignored", " --type=renderer ");
  }

  result = AddGenericPolicy(policy.get());

  if (result != sandbox::SBOX_ALL_OK) {
    NOTREACHED();
    return result;
  }

  // Allow the renderer and gpu processes to access the log file.
  if (type_str == switches::kRendererProcess ||
      type_str == switches::kGpuProcess) {
    if (logging::IsLoggingToFileEnabled()) {
      DCHECK(base::FilePath(logging::GetLogFileFullPath()).IsAbsolute());
      result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                               sandbox::TargetPolicy::FILES_ALLOW_ANY,
                               logging::GetLogFileFullPath().c_str());
      if (result != sandbox::SBOX_ALL_OK)
        return result;
    }
  }

#if !defined(OFFICIAL_BUILD)
  // If stdout/stderr point to a Windows console, these calls will
  // have no effect. These calls can fail with SBOX_ERROR_BAD_PARAMS.
  policy->SetStdoutHandle(GetStdHandle(STD_OUTPUT_HANDLE));
  policy->SetStderrHandle(GetStdHandle(STD_ERROR_HANDLE));
#endif

  if (!delegate->PreSpawnTarget(policy.get()))
    return sandbox::SBOX_ERROR_DELEGATE_PRE_SPAWN;

  TRACE_EVENT_BEGIN0("startup", "StartProcessWithAccess::LAUNCHPROCESS");

  PROCESS_INFORMATION temp_process_info = {};
  sandbox::ResultCode last_warning = sandbox::SBOX_ALL_OK;
  DWORD last_error = ERROR_SUCCESS;
  result = g_broker_services->SpawnTarget(
      cmd_line->GetProgram().value().c_str(),
      cmd_line->GetCommandLineString().c_str(), policy, &last_warning,
      &last_error, &temp_process_info);

  base::win::ScopedProcessInformation target(temp_process_info);

  TRACE_EVENT_END0("startup", "StartProcessWithAccess::LAUNCHPROCESS");

  base::debug::GlobalActivityTracker* tracker =
      base::debug::GlobalActivityTracker::Get();
  if (tracker) {
    tracker->RecordProcessLaunch(target.process_id(),
                                 cmd_line->GetCommandLineString());
  }

  if (sandbox::SBOX_ALL_OK != result) {
    UMA_HISTOGRAM_SPARSE_SLOWLY("Process.Sandbox.Launch.Error", last_error);
    if (result == sandbox::SBOX_ERROR_GENERIC)
      DPLOG(ERROR) << "Failed to launch process";
    else
      DLOG(ERROR) << "Failed to launch process. Error: " << result;

    return result;
  }

  if (sandbox::SBOX_ALL_OK != last_warning) {
    LogLaunchWarning(last_warning, last_error);
  }

  delegate->PostSpawnTarget(target.process_handle());

  CHECK(ResumeThread(target.thread_handle()) != static_cast<DWORD>(-1));
  *process = base::Process(target.TakeProcessHandle());
  return sandbox::SBOX_ALL_OK;
}

}  // namespace content
