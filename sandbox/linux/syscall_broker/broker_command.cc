// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/syscall_broker/broker_command.h"
#include "sandbox/linux/syscall_broker/broker_policy.h"

namespace sandbox {
namespace syscall_broker {

bool CommandAccessIsSafe(uint32_t mask,
                         const BrokerPolicy& policy,
                         const char* requested_filename,
                         int requested_mode,
                         const char** filename_to_use) {
  return (mask & kBrokerCommandAccessMask) &&
         policy.GetFileNameIfAllowedToAccess(requested_filename, requested_mode,
                                             filename_to_use);
}

bool CommandOpenIsSafe(uint32_t mask,
                       const BrokerPolicy& policy,
                       const char* requested_filename,
                       int requested_flags,
                       const char** filename_to_use,
                       bool* unlink_after_open) {
  return (mask & kBrokerCommandOpenMask) &&
         policy.GetFileNameIfAllowedToOpen(requested_filename, requested_flags,
                                           filename_to_use, unlink_after_open);
}

bool CommandReadlinkIsSafe(uint32_t mask,
                           const BrokerPolicy& policy,
                           const char* requested_filename,
                           const char** filename_to_use) {
  bool ignore;
  return (mask & kBrokerCommandReadlinkMask) &&
         policy.GetFileNameIfAllowedToOpen(requested_filename, O_RDONLY,
                                           filename_to_use, &ignore);
}

bool CommandRenameIsSafe(uint32_t mask,
                         const BrokerPolicy& policy,
                         const char* old_filename,
                         const char* new_filename,
                         const char** old_filename_to_use,
                         const char** new_filename_to_use) {
  bool ignore;
  return (mask & kBrokerCommandRenameMask) &&
         policy.GetFileNameIfAllowedToOpen(old_filename, O_RDWR,
                                           old_filename_to_use, &ignore) &&
         policy.GetFileNameIfAllowedToOpen(new_filename, O_RDWR,
                                           new_filename_to_use, &ignore);
}

bool CommandStatIsSafe(uint32_t mask,
                       const BrokerPolicy& policy,
                       const char* requested_filename,
                       const char** filename_to_use) {
  return (mask & kBrokerCommandStatMask) &&
         policy.GetFileNameIfAllowedToAccess(requested_filename, F_OK,
                                             filename_to_use);
}

}  // namespace syscall_broker
}  // namespace sandbox
