// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_sandbox_hook_linux.h"
#include "sandbox/linux/syscall_broker/broker_command.h"

#include "base/path_service.h"
#include "build/build_config.h"

using sandbox::syscall_broker::BrokerFilePermission;
using sandbox::syscall_broker::MakeBrokerCommandSet;

namespace content {

bool NetworkPreSandboxHook(service_manager::SandboxLinux::Options options) {
  base::FilePath home_path;
  base::PathService::Get(base::DIR_HOME, &home_path);

  auto* instance = service_manager::SandboxLinux::GetInstance();
  instance->StartBrokerProcess(
      MakeBrokerCommandSet({
          sandbox::syscall_broker::COMMAND_ACCESS,
          sandbox::syscall_broker::COMMAND_MKDIR,
          sandbox::syscall_broker::COMMAND_OPEN,
          sandbox::syscall_broker::COMMAND_READLINK,
          sandbox::syscall_broker::COMMAND_RENAME,
          sandbox::syscall_broker::COMMAND_RMDIR,
          sandbox::syscall_broker::COMMAND_STAT,
          sandbox::syscall_broker::COMMAND_UNLINK,
      }),
      {
          BrokerFilePermission::ReadOnlyRecursive("/"),
          BrokerFilePermission::ReadWriteCreateRecursive(
              home_path.AppendASCII(".pki/").value()),
          BrokerFilePermission::ReadWriteCreateRecursive(
              home_path.AppendASCII(".cache/").value()),
          BrokerFilePermission::ReadWriteCreateRecursive(
              home_path.AppendASCII(".config/chromium/").value()),
          BrokerFilePermission::ReadWriteCreateRecursive(
              home_path.AppendASCII(".config/chrome/").value()),
      },
      service_manager::SandboxLinux::PreSandboxHook(), options);

  instance->EngageNamespaceSandbox(false /* from_zygote */);
  return true;
}

}  // namespace content
