// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/syscall_broker/broker_host.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/third_party/valgrind/valgrind.h"
#include "sandbox/linux/syscall_broker/broker_common.h"
#include "sandbox/linux/syscall_broker/broker_policy.h"
#include "sandbox/linux/syscall_broker/broker_simple_message.h"
#include "sandbox/linux/system_headers/linux_syscalls.h"

namespace sandbox {

namespace syscall_broker {

namespace {

bool IsRunningOnValgrind() {
  return RUNNING_ON_VALGRIND;
}

// A little open(2) wrapper to handle some oddities for us. In the general case
// make a direct system call since we want to keep in control of the broker
// process' system calls profile to be able to loosely sandbox it.
int sys_open(const char* pathname, int flags) {
  // Hardcode mode to rw------- when creating files.
  int mode;
  if (flags & O_CREAT) {
    mode = 0600;
  } else {
    mode = 0;
  }
  if (IsRunningOnValgrind()) {
    // Valgrind does not support AT_FDCWD, just use libc's open() in this case.
    return open(pathname, flags, mode);
  } else {
    return syscall(__NR_openat, AT_FDCWD, pathname, flags, mode);
  }
}

// Open |requested_filename| with |flags| if allowed by our policy.
// Write the syscall return value (-errno) to |return_val| and append
// a file descriptor to |opened_file| if relevant.
void OpenFileForIPC(const BrokerPolicy& policy,
                    const std::string& requested_filename,
                    int flags,
                    int* return_val,
                    int* opened_file) {
  DCHECK(return_val);
  DCHECK(opened_file);
  const char* file_to_open = NULL;
  bool unlink_after_open = false;
  const bool safe_to_open_file = policy.GetFileNameIfAllowedToOpen(
      requested_filename.c_str(), flags, &file_to_open, &unlink_after_open);

  if (safe_to_open_file) {
    CHECK(file_to_open);
    int opened_fd = sys_open(file_to_open, flags);
    if (opened_fd < 0) {
      *return_val = -errno;
    } else {
      // Success.
      if (unlink_after_open) {
        unlink(file_to_open);
      }
      *opened_file = opened_fd;
      *return_val = 0;
    }
  } else {
    *return_val = -policy.denied_errno();
  }
}

// Perform access(2) on |requested_filename| with mode |mode| if allowed by our
// policy. Write the syscall return value (-errno) to |write_pickle|.
void AccessFileForIPC(const BrokerPolicy& policy,
                      const std::string& requested_filename,
                      int mode,
                      int* return_val) {
  DCHECK(return_val);
  const char* file_to_access = NULL;
  const bool safe_to_access_file = policy.GetFileNameIfAllowedToAccess(
      requested_filename.c_str(), mode, &file_to_access);

  if (safe_to_access_file) {
    CHECK(file_to_access);
    int access_ret = access(file_to_access, mode);
    int access_errno = errno;
    if (!access_ret)
      *return_val = 0;
    else
      *return_val = -access_errno;
  } else {
    *return_val = -policy.denied_errno();
  }
}

// Handle a |command_type| request contained in |iter| and send the reply
// on |reply_ipc|.
// Currently COMMAND_OPEN and COMMAND_ACCESS are supported.
bool HandleRemoteCommand(const BrokerPolicy& policy,
                         IPCCommand command_type,
                         int reply_ipc,
                         BrokerSimpleMessage* message) {
  // Currently all commands have two arguments: filename and flags.
  // Remaining message structure:
  //   char[]: filename
  //   int:    flags
  const char* data;
  int data_len;
  int flags = 0;
  if (!message->ReadData(&data, &data_len) || !message->ReadInt(&flags)) {
    return false;
  }
  std::string requested_filename(data, data_len);

  int return_val = 0;
  int opened_file = -1;

  switch (command_type) {
    case COMMAND_ACCESS:
      AccessFileForIPC(policy, requested_filename, flags, &return_val);
      break;
    case COMMAND_OPEN:
      OpenFileForIPC(policy, requested_filename, flags, &return_val,
                     &opened_file);
      break;
    default:
      LOG(ERROR) << "Invalid IPC command";
      break;
  }

  BrokerSimpleMessage reply;
  CHECK(reply.AddIntToMessage(return_val));

  ssize_t sent = BrokerSimpleMessage::SendMsg(reply_ipc, reply, opened_file);

  // Close anything we have opened in this process.
  if (opened_file >= 0) {
    int ret = IGNORE_EINTR(close(opened_file));
    DCHECK(!ret) << "Could not close file descriptor";
  }

  if (sent <= 0) {
    LOG(ERROR) << "Could not send IPC reply";
    return false;
  }
  return true;
}

}  // namespace

BrokerHost::BrokerHost(const BrokerPolicy& broker_policy,
                       BrokerChannel::EndPoint ipc_channel)
    : broker_policy_(broker_policy), ipc_channel_(std::move(ipc_channel)) {}

BrokerHost::~BrokerHost() {
}

// Handle a request on the IPC channel ipc_channel_.
// A request should have a file descriptor attached on which we will reply and
// that we will then close.
// A request should start with an int that will be used as the command type.
BrokerHost::RequestStatus BrokerHost::HandleRequest() const {
  BrokerSimpleMessage message;
  errno = 0;
  base::ScopedFD temporary_ipc;
  const ssize_t msg_len = BrokerSimpleMessage::RecvMsgWithFlags(
      ipc_channel_.get(), &message, 0, &temporary_ipc);

  if (msg_len == 0 || (msg_len == -1 && errno == ECONNRESET)) {
    // EOF from the client, or the client died, we should die.
    return RequestStatus::LOST_CLIENT;
  }

  // The client sends exactly one file descriptor, on which we will write the
  // reply.
  if (msg_len < 0) {
    PLOG(ERROR) << "Error reading message from the client";
    return RequestStatus::FAILURE;
  }

  // Message structure:
  //   int:    command type
  //   char[]: pathname
  //   int:    flags
  int command_type;
  if (message.ReadInt(&command_type)) {
    bool command_handled = false;
    // Go through all the possible IPC messages.
    switch (command_type) {
      case COMMAND_ACCESS:
      case COMMAND_OPEN:
        // We reply on the file descriptor sent to us via the IPC channel.
        command_handled = HandleRemoteCommand(
            broker_policy_, static_cast<IPCCommand>(command_type),
            temporary_ipc.get(), &message);
        break;
      default:
        NOTREACHED();
        break;
    }

    if (command_handled) {
      return RequestStatus::SUCCESS;
    } else {
      return RequestStatus::FAILURE;
    }

    NOTREACHED();
  }

  LOG(ERROR) << "Error parsing IPC request";
  return RequestStatus::FAILURE;
}

}  // namespace syscall_broker

}  // namespace sandbox
