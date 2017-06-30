// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_receiver_pipe_posix.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket_linux.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "chrome/profiling/memlog_stream_receiver.h"

namespace profiling {

namespace {

// Use a large buffer for our pipe. We don't want the sender to block
// if at all possible since it will slow the app down quite a bit.
//
// TODO(ajwong): Figure out how to size this. Currently the number is cribbed
// from the right size for windows named pipes.
const int kReadBufferSize = 1024 * 64;

}  // namespace

MemlogReceiverPipe::MemlogReceiverPipe(base::ScopedFD fd)
    : fd_(std::move(fd)), read_buffer_(new char[kReadBufferSize]) {
  static std::vector<base::ScopedFD> dummy_instance;
  dummy_for_receive_ = &dummy_instance;
}

MemlogReceiverPipe::~MemlogReceiverPipe() {}

void MemlogReceiverPipe::ReadUntilBlocking() {
  // TODO(brettw) note that the IO completion callback will always be issued,
  // even for sync returns of ReadFile. If there is a lot of data ready to be
  // read, it would be nice to process them all in this loop rather than having
  // to go back to the message loop for each block, but that will require
  // different IOContext structures for each one.
  ssize_t bytes_read = 0;
  do {
    bytes_read = base::UnixDomainSocket::RecvMsg(
        fd_.get(), read_buffer_.get(), kReadBufferSize, dummy_for_receive_);
    if (bytes_read > 0) {
      receiver_task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&MemlogStreamReceiver::OnStreamData, receiver_,
                         std::move(read_buffer_), bytes_read));
      read_buffer_.reset(new char[kReadBufferSize]);
      return;
    } else if (bytes_read == 0) {
      // Other end closed the pipe.
      if (receiver_) {
        receiver_task_runner_->PostTask(
            FROM_HERE,
            base::BindOnce(&MemlogStreamReceiver::OnStreamComplete, receiver_));
      }
      return;
    } else {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        PLOG(ERROR) << "Problem reading socket.";
      }
    }
  } while (bytes_read > 0);
}

int MemlogReceiverPipe::GetRemoteProcessID() {
  return 1;  // TODO(ajwong): Record the origin ID somehow.
}

void MemlogReceiverPipe::SetReceiver(
    scoped_refptr<base::TaskRunner> task_runner,
    scoped_refptr<MemlogStreamReceiver> receiver) {
  receiver_task_runner_ = task_runner;
  receiver_ = receiver;
}

}  // namespace profiling
