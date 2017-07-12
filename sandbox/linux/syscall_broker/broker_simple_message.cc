// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/syscall_broker/broker_simple_message.h"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket_linux.h"
#include "base/process/process_handle.h"
#include "build/build_config.h"

namespace sandbox {

namespace syscall_broker {

BrokerSimpleMessage::BrokerSimpleMessage()
    : length_(0),
      read_next_(message_),
      write_next_(message_),
      read_only_(false),
      write_only_(false) {}

// static
ssize_t BrokerSimpleMessage::SendRecvMsgWithFlags(
    int fd,
    BrokerSimpleMessage* reply,
    int recvmsg_flags,
    int* result_fd,
    const BrokerSimpleMessage& message) {
  CHECK(reply);
  // This socketpair is only used for the IPC and is cleaned up before
  // returning.
  base::ScopedFD recv_sock, send_sock;
  if (!base::CreateSocketPair(&recv_sock, &send_sock))
    return -1;

  {
    int send_fd;
    send_fd = send_sock.get();
    if (!SendMsg(fd, message, send_fd))
      return -1;
  }

  // Close the sending end of the socket right away so that if our peer closes
  // it before sending a response (e.g., from exiting), RecvMsgWithFlags() will
  // return EOF instead of hanging.
  send_sock.reset();

  base::ScopedFD recv_fd;
  // When porting to OSX keep in mind it doesn't support MSG_NOSIGNAL, so the
  // sender might get a SIGPIPE.
  const ssize_t reply_len =
      RecvMsgWithFlags(recv_sock.get(), reply, recvmsg_flags, &recv_fd);
  recv_sock.reset();
  if (reply_len == -1)
    return -1;

  if (result_fd)
    *result_fd = (recv_fd == -1) ? -1 : recv_fd.release();

  return reply_len;
}

// static
bool BrokerSimpleMessage::SendMsg(int fd,
                                  const BrokerSimpleMessage& message,
                                  int send_fd) {
  struct msghdr msg = {};
  const void* buf = reinterpret_cast<const void*>(message.message_);
  struct iovec iov = {const_cast<void*>(buf), message.length_};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  const unsigned control_len = CMSG_SPACE(sizeof(int));
  char control_buffer[control_len];
  if (send_fd >= 0) {
    struct cmsghdr* cmsg;
    msg.msg_control = control_buffer;
    msg.msg_controllen = control_len;
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &send_fd, sizeof(int));
    msg.msg_controllen = cmsg->cmsg_len;
  }

  // Avoid a SIGPIPE if the other end breaks the connection.
  // Due to a bug in the Linux kernel (net/unix/af_unix.c) MSG_NOSIGNAL isn't
  // regarded for SOCK_SEQPACKET in the AF_UNIX domain, but it is mandated by
  // POSIX.
  const int flags = MSG_NOSIGNAL;
  const ssize_t r = HANDLE_EINTR(sendmsg(fd, &msg, flags));
  const bool ret = static_cast<ssize_t>(message.length_) == r;
  return ret;
}

// static
ssize_t BrokerSimpleMessage::RecvMsgWithFlags(int fd,
                                              BrokerSimpleMessage* message,
                                              int flags,
                                              base::ScopedFD* return_fd) {
  // The message must be fresh and unused
  CHECK(message && !message->read_only_ && !message->write_only_);
  message->read_only_ = true;  // message should not be written to again
  struct msghdr msg = {};
  struct iovec iov = {message->message_, kMaxMessageLength};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  const size_t kControlBufferSize =
      CMSG_SPACE(sizeof(int) * base::UnixDomainSocket::kMaxFileDescriptors)
#if !defined(OS_NACL_NONSFI)
      // The PNaCl toolchain for Non-SFI binary build does not support ucred.
      + CMSG_SPACE(sizeof(struct ucred))
#endif
      ;
  char control_buffer[kControlBufferSize];
  msg.msg_control = control_buffer;
  msg.msg_controllen = sizeof(control_buffer);

  const ssize_t r = HANDLE_EINTR(recvmsg(fd, &msg, flags));
  if (r == -1)
    return -1;

  int* wire_fds = NULL;
  unsigned wire_fds_len = 0;
  base::ProcessId pid = -1;

  if (msg.msg_controllen > 0) {
    struct cmsghdr* cmsg;
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
      const unsigned payload_len = cmsg->cmsg_len - CMSG_LEN(0);
      if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
        DCHECK_EQ(payload_len % sizeof(int), 0u);
        DCHECK_EQ(wire_fds, static_cast<void*>(nullptr));
        wire_fds = reinterpret_cast<int*>(CMSG_DATA(cmsg));
        wire_fds_len = payload_len / sizeof(int);
      }
#if !defined(OS_NACL_NONSFI)
      // The PNaCl toolchain for Non-SFI binary build does not support
      // SCM_CREDENTIALS.
      if (cmsg->cmsg_level == SOL_SOCKET &&
          cmsg->cmsg_type == SCM_CREDENTIALS) {
        DCHECK_EQ(payload_len, sizeof(struct ucred));
        DCHECK_EQ(pid, -1);
        pid = reinterpret_cast<struct ucred*>(CMSG_DATA(cmsg))->pid;
      }
#endif
    }
  }

  if (msg.msg_flags & MSG_TRUNC || msg.msg_flags & MSG_CTRUNC) {
    for (unsigned i = 0; i < wire_fds_len; ++i)
      close(wire_fds[i]);
    errno = EMSGSIZE;
    return -1;
  }

  if (wire_fds) {
    if (wire_fds_len > 1) {
      // Only one FD is accepted by this receive.
      for (unsigned i = 0; i < wire_fds_len; ++i)
        close(wire_fds[i]);
      errno = EMSGSIZE;
      NOTREACHED();
      return -1;
    }

    *return_fd = base::ScopedFD(wire_fds[0]);
  }

  // At this point, |r| is guaranteed to be >= 0.
  message->length_ = static_cast<size_t>(r);
  return r;
}

bool BrokerSimpleMessage::AddDataToMessage(const char* data, int length) {
  CHECK(!read_only_);
  write_only_ = true;  // message should only be written to going forward

  if ((length_ + length + sizeof(EntryType) + sizeof(int)) > kMaxMessageLength)
    return false;

  EntryType type = EntryType::DATA;

  // Write the type to the message
  memcpy(write_next_, &type, sizeof(EntryType));
  write_next_ = write_next_ + sizeof(EntryType);
  // Write the length of the buffer to the message
  memcpy(write_next_, &length, sizeof(int));
  write_next_ = write_next_ + sizeof(int);
  // Write the data in the buffer to the message
  memcpy(write_next_, data, sizeof(char) * length);
  write_next_ = write_next_ + sizeof(char) * length;
  length_ = write_next_ - message_;

  return true;
}

bool BrokerSimpleMessage::AddIntToMessage(int data) {
  CHECK(!read_only_);
  write_only_ = true;  // message should only be written to going forward

  if ((length_ + sizeof(int)) > kMaxMessageLength)
    return false;

  EntryType type = EntryType::INT;

  memcpy(write_next_, &type, sizeof(EntryType));
  write_next_ = write_next_ + sizeof(EntryType);
  memcpy(write_next_, &data, sizeof(int));
  write_next_ = write_next_ + sizeof(int);
  length_ = write_next_ - message_;

  return true;
}

bool BrokerSimpleMessage::ReadData(const char** data, int* length) {
  CHECK(!write_only_);
  read_only_ = true;  // message should not be written to
  CHECK(read_next_ < (message_ + length_));
  uint8_t* orig_read_pos = read_next_;

  if (!ValidateType(EntryType::DATA)) {
    read_next_ = orig_read_pos;
    return false;
  }

  // Get the length of the data buffer from the message
  if ((read_next_ + sizeof(int)) > (message_ + length_)) {
    read_next_ = orig_read_pos;
    return false;
  }
  memcpy(length, read_next_, sizeof(int));
  read_next_ = read_next_ + sizeof(int);

  // Get the raw data buffer from the message
  if ((read_next_ + sizeof(char) * *length) > (message_ + length_)) {
    read_next_ = orig_read_pos;
    return false;
  }
  *data = reinterpret_cast<char*>(read_next_);
  read_next_ = read_next_ + sizeof(char) * *length;
  return true;
}

bool BrokerSimpleMessage::ReadInt(int* result) {
  CHECK(!write_only_);
  read_only_ = true;  // message should not be written to
  CHECK(read_next_ < (message_ + length_));
  uint8_t* orig_read_pos = read_next_;

  if (!ValidateType(EntryType::INT)) {
    read_next_ = orig_read_pos;
    return false;
  }

  if ((read_next_ + sizeof(int)) > (message_ + length_)) {
    read_next_ = orig_read_pos;
    return false;
  }
  memcpy(result, read_next_, sizeof(int));
  read_next_ = read_next_ + sizeof(int);
  return true;
}

bool BrokerSimpleMessage::ValidateType(EntryType expected_type) {
  if ((read_next_ + sizeof(EntryType)) > (message_ + length_))
    return false;

  EntryType type;
  memcpy(&type, read_next_, sizeof(EntryType));
  if (type != expected_type)
    return false;

  read_next_ = read_next_ + sizeof(EntryType);
  return true;
}

}  // namespace syscall_broker

}  // namespace sandbox
