// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_sandbox_hook_linux.h"
#include "sandbox/linux/syscall_broker/broker_command.h"

#include "base/rand_util.h"
#include "base/sys_info.h"

using sandbox::syscall_broker::BrokerFilePermission;

namespace content {

bool NetworkPreSandboxHook(service_manager::BPFBasePolicy* policy,
                           service_manager::SandboxLinux::Options options) {
  // TODO(tsepez): FIX THIS.
  std::vector<BrokerFilePermission> file_permissions = {
      BrokerFilePermission::ReadWriteCreateRecursive("/")};

  constexpr uint32_t allowed_command_mask =
      sandbox::syscall_broker::kBrokerCommandAccessMask |
      sandbox::syscall_broker::kBrokerCommandOpenMask |
      sandbox::syscall_broker::kBrokerCommandReadlinkMask |
      sandbox::syscall_broker::kBrokerCommandRenameMask |
      sandbox::syscall_broker::kBrokerCommandStatMask;

  service_manager::SandboxLinux::GetInstance()->StartBrokerProcess(
      policy, allowed_command_mask, std::move(file_permissions),
      service_manager::SandboxLinux::PreSandboxHook(), options);

  return true;
}

}  // namespace content
