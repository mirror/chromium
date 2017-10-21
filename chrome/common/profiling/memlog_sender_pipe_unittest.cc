// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX)
#include "chrome/common/profiling/memlog_sender_pipe_posix.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#else
#include "chrome/common/profiling/memlog_sender_pipe_win.h"
#endif

namespace profiling {
namespace {

using Result = MemlogSenderPipe::Result;

class MemlogSenderPipeTest : public testing::Test {
 public:
  void SetUp() override {
    mojo::edk::ScopedPlatformHandle write_handle;

// The pipe endpoints are created and brokered by the ProfilingProcessHost.
// This directory is a dependency of ProfilingProcessHost. While we could
// move this test into ProfilingProcessHost to share the logic for creating
// pipe endpoints, the logic being tested is that of the MemlogSenderPipe,
// so this location seems more suitable. The amount of duplicated logic is
// minimal.
#if defined(OS_MACOSX)
    int fds[2];
    pipe(fds);
    PCHECK(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
    PCHECK(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);
    PCHECK(fcntl(fds[0], F_SETNOSIGPIPE, 1) == 0);
    PCHECK(fcntl(fds[1], F_SETNOSIGPIPE, 1) == 0);
    write_handle.reset(mojo::edk::PlatformHandle(fds[1]));
    read_handle_.reset(mojo::edk::PlatformHandle(fds[0]));
#else
    mojo::edk::PlatformChannelPair data_channel(false /* client_is_blocking */);
    write_handle = data_channel.PassClientHandle();
    read_handle_ = data_channel.PassServerHandle();
#endif

    base::ScopedPlatformFile file(write_handle.release().handle);
    sender_pipe_.reset(new MemlogSenderPipe(std::move(file)));

    // A large buffer for both writing and reading.
    buffer_.resize(64 * 1024);
  }

  Result Write(int size) { return sender_pipe_->Send(buffer_.data(), size, 1); }

  void Read(int size) {
#if defined(OS_POSIX)
    ssize_t bytes_read = read(read_handle_.get().handle, buffer_.data(), size);
#else
    OVERLAPPED overlapped;
    DWORD bytes_read = 0;
    memset(&overlapped, 0, sizeof(OVERLAPPED));
    BOOL result = ::ReadFile(read_handle_.get().handle, buffer_.data(), size,
                             &bytes_read, &overlapped);
    ASSERT(result);
#endif
    ASSERT_EQ(size, bytes_read);
  }

 private:
  mojo::edk::ScopedPlatformHandle read_handle_;
  std::unique_ptr<MemlogSenderPipe> sender_pipe_;
  std::vector<char> buffer_;
};

TEST_F(MemlogSenderPipeTest, TimeoutNoRead) {
  // Writing 64k should not time out.
  Result result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);

  // Writing 64k more should time out, since the buffer size is 64k.
  result = Write(64 * 1024);
  ASSERT_EQ(Result::kTimeout, result);
}

TEST_F(MemlogSenderPipeTest, TimeoutSmallRead) {
  // Writing 64k should not time out.
  Result result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);

  // Read 32k out of the buffer.
  Read(32 * 1024);

  // Writing 64k more should still time out, since the buffer size should be
  // 64k.
  result = Write(64 * 1024);
  ASSERT_EQ(Result::kTimeout, result);
}

TEST_F(MemlogSenderPipeTest, NoTimeout) {
  // Writing 64k should not time out.
  Result result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);

  // Read 64k out of the buffer.
  Read(64 * 1024);

  // Writing 64k should not time out.
  result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);
}

}  // namespace
}  // namespace profiling
