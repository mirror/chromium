// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/sandbox_linux/bpf_network_policy_linux.h"

#include <errno.h>

#include "build/build_config.h"
#include "content/common/sandbox_linux/sandbox_linux.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/seccomp-bpf-helpers/syscall_parameters_restrictions.h"
#include "sandbox/linux/seccomp-bpf-helpers/syscall_sets.h"
#include "sandbox/linux/system_headers/linux_syscalls.h"

using sandbox::SyscallSets;
using sandbox::bpf_dsl::Allow;
using sandbox::bpf_dsl::Error;
using sandbox::bpf_dsl::ResultExpr;

namespace content {

NetworkProcessPolicy::NetworkProcessPolicy() {}
NetworkProcessPolicy::~NetworkProcessPolicy() {}

ResultExpr NetworkProcessPolicy::EvaluateSyscall(int sysno) const {
  // TODO(tsepez): Actually restrict syscalls
  return Allow();
}

}  // namespace content
