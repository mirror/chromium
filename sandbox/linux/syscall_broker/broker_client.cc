// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/syscall_broker/broker_client.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

#include "base/logging.h"
#include "build/build_config.h"
#include "sandbox/linux/syscall_broker/broker_channel.h"
#include "sandbox/linux/syscall_broker/broker_common.h"
#include "sandbox/linux/syscall_broker/broker_policy.h"
#include "sandbox/linux/syscall_broker/broker_simple_message.h"

#if defined(OS_ANDROID) && !defined(MSG_CMSG_CLOEXEC)
#define MSG_CMSG_CLOEXEC 0x40000000
#endif

namespace sandbox {

namespace syscall_broker {

// Make a remote system call over IPC for syscalls that take a path and flags
// as arguments, currently open() and access().
// Will return -errno like a real system call.
// This function needs to be async signal safe.
int BrokerClient::PathAndFlagsSyscall(IPCCommand syscall_type,
                                      const char* pathname,
                                      int flags) const {
  int recvmsg_flags = 0;
  RAW_CHECK(syscall_type == COMMAND_OPEN || syscall_type == COMMAND_ACCESS);
  if (!pathname)
    return -EFAULT;

  // For this "remote system call" to work, we need to handle any flag that
  // cannot be sent over a Unix socket in a special way.
  // See the comments around kCurrentProcessOpenFlagsMask.
  if (syscall_type == COMMAND_OPEN && (flags & kCurrentProcessOpenFlagsMask)) {
    // This implementation only knows about O_CLOEXEC, someone needs to look at
    // this code if other flags are added.
    RAW_CHECK(kCurrentProcessOpenFlagsMask == O_CLOEXEC);
    recvmsg_flags |= MSG_CMSG_CLOEXEC;
    flags &= ~O_CLOEXEC;
  }

  // There is no point in forwarding a request that we know will be denied.
  // Of course, the real security check needs to be on the other side of the
  // IPC.
  if (fast_check_in_client_) {
    if (syscall_type == COMMAND_OPEN &&
        !broker_policy_.GetFileNameIfAllowedToOpen(
            pathname, flags, NULL /* file_to_open */,
            NULL /* unlink_after_open */)) {
      return -broker_policy_.denied_errno();
    }
    if (syscall_type == COMMAND_ACCESS &&
        !broker_policy_.GetFileNameIfAllowedToAccess(pathname, flags, NULL)) {
      return -broker_policy_.denied_errno();
    }
  }

  // Message structure:
  //   int:    syscall_type
  //   char[]: pathname, including '\0' terminator
  //   int:    flags
  BrokerSimpleMessage message;
  CHECK(message.AddIntToMessage(syscall_type));
  CHECK(message.AddDataToMessage(pathname, strlen(pathname) + 1));
  CHECK(message.AddIntToMessage(flags));

  int returned_fd = -1;
  BrokerSimpleMessage reply;

  // Send a request as well that will include a new temporary socketpair
  // (created internally by SendRecvMsg()).  Then read the reply on this new
  // socketpair in |reply| and put an eventual attached file descriptor in
  // |returned_fd|.
  ssize_t msg_len = BrokerSimpleMessage::SendRecvMsgWithFlags(
      ipc_channel_.get(), &reply, recvmsg_flags, &returned_fd, message);
  if (msg_len <= 0) {
    if (!quiet_failures_for_tests_)
      RAW_LOG(ERROR, "Could not make request to broker process");
    return -ENOMEM;
  }

  // Reply should be a simple integer type.
  int return_value;
  if (!reply.ReadInt(&return_value)) {
    RAW_LOG(ERROR, "Could not read reply");
    NOTREACHED();
    return -ENOMEM;
  }

  switch (syscall_type) {
    case COMMAND_ACCESS:
      // We should never have a fd to return.
      RAW_CHECK(returned_fd == -1);
      return return_value;
    case COMMAND_OPEN:
      if (return_value < 0) {
        RAW_CHECK(returned_fd == -1);
        return return_value;
      } else {
        // We have a real file descriptor to return.
        RAW_CHECK(returned_fd >= 0);
        return returned_fd;
      }
    default:
      RAW_LOG(ERROR, "Unsupported command");
      return -ENOSYS;
  }
}

BrokerClient::BrokerClient(const BrokerPolicy& broker_policy,
                           BrokerChannel::EndPoint ipc_channel,
                           bool fast_check_in_client,
                           bool quiet_failures_for_tests)
    : broker_policy_(broker_policy),
      ipc_channel_(std::move(ipc_channel)),
      fast_check_in_client_(fast_check_in_client),
      quiet_failures_for_tests_(quiet_failures_for_tests) {}

BrokerClient::~BrokerClient() {
}

int BrokerClient::Access(const char* pathname, int mode) const {
  return PathAndFlagsSyscall(COMMAND_ACCESS, pathname, mode);
}

int BrokerClient::Open(const char* pathname, int flags) const {
  return PathAndFlagsSyscall(COMMAND_OPEN, pathname, flags);
}

}  // namespace syscall_broker

}  // namespace sandbox
