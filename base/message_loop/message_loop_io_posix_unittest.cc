// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/compiler_specific.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/posix/eintr_wrapper.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

#if !defined(OS_NACL)

namespace {

class MessageLoopIoPosixTest : public testing::Test {
 public:
  MessageLoopIoPosixTest() {}

  // testing::Test interface.
  void SetUp() override {
    // Create a file descriptor.  Doesn't need to be readable or writable,
    // as we don't need to actually get any notifications.
    // pipe() is just the easiest way to do it.
    int pipefds[2];
    int err = pipe(pipefds);
    ASSERT_EQ(0, err);
    read_fd_ = base::File(pipefds[0]);
    write_fd_ = base::File(pipefds[1]);
  }

  base::File read_fd_;
  base::File write_fd_;

  DISALLOW_COPY_AND_ASSIGN(MessageLoopIoPosixTest);
};

class QuitDelegate : public MessageLoopForIO::Watcher {
 public:
  void OnFileCanWriteWithoutBlocking(int fd) override {
    MessageLoop::current()->QuitWhenIdle();
  }
  void OnFileCanReadWithoutBlocking(int fd) override {
    MessageLoop::current()->QuitWhenIdle();
  }
};

TEST_F(MessageLoopIoPosixTest, FileDescriptorWatcherOutlivesMessageLoop) {
  // Simulate a MessageLoop that dies before an FileDescriptorWatcher.
  // This could happen when people use the Singleton pattern or atexit.

  // Arrange for watcher to live longer than message loop.
  MessageLoopForIO::FileDescriptorWatcher watcher(FROM_HERE);
  {
    MessageLoopForIO message_loop;

    QuitDelegate delegate;
    message_loop.WatchFileDescriptor(write_fd_.GetPlatformFile(), true,
                                     MessageLoopForIO::WATCH_WRITE, &watcher,
                                     &delegate);
    // Don't run the message loop, just destroy it.
  }
}

TEST_F(MessageLoopIoPosixTest, FileDescriptorWatcherDoubleStop) {
  // Verify that it's ok to call StopWatchingFileDescriptor().
  // (Errors only showed up in valgrind.)

  // Arrange for message loop to live longer than watcher.
  MessageLoopForIO message_loop;
  {
    MessageLoopForIO::FileDescriptorWatcher watcher(FROM_HERE);

    QuitDelegate delegate;
    message_loop.WatchFileDescriptor(write_fd_.GetPlatformFile(), true,
                                     MessageLoopForIO::WATCH_WRITE, &watcher,
                                     &delegate);
    ASSERT_TRUE(watcher.StopWatchingFileDescriptor());
    ASSERT_TRUE(watcher.StopWatchingFileDescriptor());
  }
}

class DeleteDelegate : public MessageLoopForIO::Watcher {
 public:
  DeleteDelegate(
      std::unique_ptr<MessageLoopForIO::FileDescriptorWatcher> watcher)
      : watcher_(std::move(watcher)) {}
  void OnFileCanWriteWithoutBlocking(int fd) override {
    watcher_ = nullptr;
    MessageLoop::current()->QuitWhenIdle();
  }
  void OnFileCanReadWithoutBlocking(int fd) override {
    watcher_ = nullptr;
    MessageLoop::current()->QuitWhenIdle();
  }
  MessageLoopForIO::FileDescriptorWatcher* watcher() { return watcher_.get(); }

 private:
  std::unique_ptr<MessageLoopForIO::FileDescriptorWatcher> watcher_;
};

TEST_F(MessageLoopIoPosixTest, FileDescriptorWatcherDeleteInCallback) {
  // Verify that it is OK to delete the FileDescriptorWatcher from within a
  // callback.
  MessageLoopForIO message_loop;

  DeleteDelegate delegate(
      base::MakeUnique<MessageLoopForIO::FileDescriptorWatcher>(FROM_HERE));

  message_loop.WatchFileDescriptor(write_fd_.GetPlatformFile(), true,
                                   MessageLoopForIO::WATCH_WRITE,
                                   delegate.watcher(), &delegate);
  RunLoop().Run();
}

class RecordNotificationAndQuitWatcher : public MessageLoopForIO::Watcher {
 public:
  void OnFileCanReadWithoutBlocking(int fd) override {
    is_readable_ = true;
    MessageLoop::current()->QuitWhenIdle();
  }
  void OnFileCanWriteWithoutBlocking(int fd) override {
    is_writable_ = true;
    MessageLoop::current()->QuitWhenIdle();
  }

  bool is_readable_ = false;
  bool is_writable_ = false;
};

// Verify that basic readable notification works.
TEST_F(MessageLoopIoPosixTest, WatchReadable) {
  MessageLoopForIO message_loop;
  MessageLoopForIO::FileDescriptorWatcher watcher(FROM_HERE);
  RecordNotificationAndQuitWatcher delegate;

  // Watch the pipe for readability.
  ASSERT_TRUE(MessageLoopForIO::current()->WatchFileDescriptor(
      read_fd_.GetPlatformFile(), /* persistent= */ false,
      MessageLoopForIO::WATCH_READ, &watcher, &delegate));

  // The pipe should not be readable when first created.
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(delegate.is_readable_);
  ASSERT_FALSE(delegate.is_writable_);

  // Write a byte to the other end, making it readable.
  const char buf = 0;
  ASSERT_TRUE(
      WriteFileDescriptor(write_fd_.GetPlatformFile(), &buf, sizeof(buf)));

  // We don't want to assume that the read fd becomes readable the
  // instant a bytes is written, so Run until quit by an event.
  RunLoop().Run();

  ASSERT_TRUE(delegate.is_readable_);
  ASSERT_FALSE(delegate.is_writable_);
}

// Verify that watching a file descriptor for writability succeeds.
TEST_F(MessageLoopIoPosixTest, WatchWritable) {
  MessageLoopForIO message_loop;
  MessageLoopForIO::FileDescriptorWatcher watcher(FROM_HERE);
  RecordNotificationAndQuitWatcher delegate;

  // Watch the pipe for writability.
  ASSERT_TRUE(MessageLoopForIO::current()->WatchFileDescriptor(
      write_fd_.GetPlatformFile(), /* persistent= */ false,
      MessageLoopForIO::WATCH_WRITE, &watcher, &delegate));

  // We should not receive a writable notification until we process events.
  ASSERT_FALSE(delegate.is_readable_);
  ASSERT_FALSE(delegate.is_writable_);

  // The pipe should be writable immediately, but wait for the quit closure
  // anyway, to be sure.
  RunLoop().Run();

  ASSERT_FALSE(delegate.is_readable_);
  ASSERT_TRUE(delegate.is_writable_);
}

}  // namespace

#endif  // !defined(OS_NACL)

}  // namespace base
