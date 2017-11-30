// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SYSCALL_BROKER_BROKER_COMMAND_H_
#define SANDBOX_LINUX_SYSCALL_BROKER_BROKER_COMMAND_H_

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

namespace sandbox {
namespace syscall_broker {

class BrokerPolicy;

constexpr size_t kMaxMessageLength = 4096;

// Some flags are local to the current process and cannot be sent over a Unix
// socket. They need special treatment from the client.
// O_CLOEXEC is tricky because in theory another thread could call execve()
// before special treatment is made on the client, so a client needs to call
// recvmsg(2) with MSG_CMSG_CLOEXEC.
// To make things worse, there are two CLOEXEC related flags, FD_CLOEXEC (see
// F_GETFD in fcntl(2)) and O_CLOEXEC (see F_GETFL in fcntl(2)). O_CLOEXEC
// doesn't affect the semantics on execve(), it's merely a note that the
// descriptor was originally opened with O_CLOEXEC as a flag. And it is sent
// over unix sockets just fine, so a receiver that would (incorrectly) look at
// O_CLOEXEC instead of FD_CLOEXEC may be tricked in thinking that the file
// descriptor will or won't be closed on execve().
constexpr int kCurrentProcessOpenFlagsMask = O_CLOEXEC;

enum BrokerCommand {
  COMMAND_INVALID = 0,
  COMMAND_ACCESS,
  COMMAND_OPEN,
  COMMAND_READLINK,
  COMMAND_RENAME,
  COMMAND_STAT,
  COMMAND_STAT64,
};

constexpr uint32_t kBrokerCommandInvalidMask = 0;
constexpr uint32_t kBrokerCommandAllMask = 0xffffffff;
constexpr uint32_t kBrokerCommandAccessMask = 1u << COMMAND_ACCESS;
constexpr uint32_t kBrokerCommandOpenMask = 1u << COMMAND_OPEN;
constexpr uint32_t kBrokerCommandReadlinkMask = 1u << COMMAND_READLINK;
constexpr uint32_t kBrokerCommandRenameMask = 1u << COMMAND_RENAME;
constexpr uint32_t kBrokerCommandStatMask = 1u << COMMAND_STAT;

// Helper functions to perform the same permissions test on either side.
bool CommandAccessIsSafe(uint32_t mask,
                         const BrokerPolicy& policy,
                         const char* requested_filename,
                         int requested_mode,  // e.g. F_OK, R_OK, W_OK.
                         const char** filename_to_use);

bool CommandOpenIsSafe(uint32_t mask,
                       const BrokerPolicy& policy,
                       const char* requested_filename,
                       int requested_flags,  // e.g. O_RDONLY, O_RDWR.
                       const char** filename_to_use,
                       bool* unlink_after_open);

bool CommandReadlinkIsSafe(uint32_t mask,
                           const BrokerPolicy& policy,
                           const char* requested_filename,
                           const char** filename_to_use);

bool CommandRenameIsSafe(uint32_t mask,
                         const BrokerPolicy& policy,
                         const char* old_filename,
                         const char* new_filename,
                         const char** old_filename_to_use,
                         const char** new_filename_to_use);

bool CommandStatIsSafe(uint32_t mask,
                       const BrokerPolicy& policy,
                       const char* requested_filename,
                       const char** filename_to_use);

}  // namespace syscall_broker
}  // namespace sandbox

#endif  // SANDBOX_LINUX_SYSCALL_BROKER_BROKER_COMMAND_H_
