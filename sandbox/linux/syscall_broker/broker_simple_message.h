// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SYSCALL_BROKER_BROKER_BASIC_SENDRECVMSG_H_
#define SANDBOX_LINUX_SYSCALL_BROKER_BROKER_BASIC_SENDRECVMSG_H_

#include <stdint.h>
#include <sys/types.h>

#include "base/files/scoped_file.h"
#include "sandbox/linux/syscall_broker/broker_common.h"

namespace sandbox {

namespace syscall_broker {

// This class is meant to provide a very simple messaging mechanism that is
// signal-safe for the broker to utilize. This addresses many of the issues
// outlined in https://crbug.com/255063. In short, the use of the standard
// base::UnixDomainSockets is not possible because it uses base::Pickle and
// std::vector, which are not signal-safe.
//
// In implementation, much of the code for sending and receiving is taken from
// base::UnixDomainSockets and re-used below. Thus, ultimately, it might be
// worthwhile making a first-class base-supported signal-safe set of mechanisms
// that reduces the code duplication.
class BrokerSimpleMessage {
 public:
  BrokerSimpleMessage();

  // Signal-safe
  static ssize_t SendRecvMsgWithFlags(int fd,
                                      BrokerSimpleMessage* reply,
                                      int recvmsg_flags,
                                      int* send_fd,
                                      const BrokerSimpleMessage& message);

  // Use sendmsg to write the given msg and include an array of file
  // descriptors. Returns true if successful. Signal-safe.
  static bool SendMsg(int fd, const BrokerSimpleMessage& message, int send_fd);

  // Similar to RecvMsg, but allows to specify |flags| for recvmsg(2).
  // Guaranteed to return either 1 or 0 fds. Signal-safe.
  static ssize_t RecvMsgWithFlags(int fd,
                                  BrokerSimpleMessage* message,
                                  int flags,
                                  base::ScopedFD* return_fd);

  // Adds a raw data buffer to the message. If the raw data is actually a
  // string, be sure to have length explicitly include the '\0' terminating
  // character.
  bool AddDataToMessage(const char*, int length);
  bool AddIntToMessage(int);

  bool ReadData(const char** data, int* length);
  bool ReadInt(int* result);

 private:
  friend class BrokerSimpleMessageTestHelper;

  enum class EntryType { DATA = 0, INT };

  bool ValidateType(EntryType expected_type);

  uint8_t message_[kMaxMessageLength];
  size_t length_;
  uint8_t* read_next_;
  uint8_t* write_next_;

  bool read_only_;
  bool write_only_;
};

}  // namespace syscall_broker

}  // namespace sandbox

#endif  // SANDBOX_LINUX_SYSCALL_BROKER_BROKER_BASIC_SENDRECVMSG_H_
