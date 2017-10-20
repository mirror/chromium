// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender_pipe_posix.h"

#include "testing/gtest/include/gtest/gtest.h"
namespace profiling {
namespace {

using Result = MemlogSenderPipe::Result;

TEST(MemlogSenderPipe, Timeout) {
  int write_pipe = 0;
  int read_pipe = 0;

  // The pipe endpoints are created and brokered by the ProfilingProcessHost. We
  // cannot include that logic in this directory, so instead we duplicate the
  // logic here.
#if defined(OS_MACOSX)
  int fds[2];
  pipe(fds);
  PCHECK(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
  PCHECK(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);
  PCHECK(fcntl(fds[0], F_SETNOSIGPIPE, 1) == 0);
  PCHECK(fcntl(fds[1], F_SETNOSIGPIPE, 1) == 0);
  write_pipe = fds[1];
  read_pipe = fds[0];
#else
  mojo::edk::PlatformChannelPair data_channel(false /* client_is_blocking */);
  write_pipe = data_channel.PassClientHandle().release().handle;
  read_pipe = data_channel.PassServerHandle().release().handle;
#endif

  base::ScopedPlatformFile file(write_pipe);
  MemlogSenderPipe sender_pipe(std::move(file));
  std::vector<char> buffer;
  buffer.resize(64 * 1024);

  // Writing 64k should not time out.
  Result result =  sender_pipe.Send(buffer.data(), buffer.size(), 1);
  ASSERT_EQ(Result::kSuccess, result);

  // Writing 64k more should time out.
  result =  sender_pipe.Send(buffer.data(), buffer.size(), 1);
  ASSERT_EQ(Result::kTimeout, result);

  // Read 32k out of the buffer.
  ssize_t bytes_read = read(read_pipe, buffer.data(), 32 * 1024);
  ASSERT_EQ(32 * 1024, bytes_read);

  // Writing 64k more should still time out, although 32k will have been
  // written.
  result =  sender_pipe.Send(buffer.data(), buffer.size(), 1);
  ASSERT_EQ(Result::kTimeout, result);

  // Read another 64k out of the buffer.
  bytes_read = read(read_pipe, buffer.data(), 64 * 1024);
  ASSERT_EQ(64 * 1024, bytes_read);

  // Writing 64k should not time out.
  result =  sender_pipe.Send(buffer.data(), buffer.size(), 1);
  ASSERT_EQ(Result::kSuccess, result);
}

}  // namespace
}  // namespace profiling
