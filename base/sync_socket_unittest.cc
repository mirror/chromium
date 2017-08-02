// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/sync_socket.h"

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

constexpr TimeDelta kReceiveTimeout = base::TimeDelta::FromMilliseconds(750);

// Time to sleep before the tests can assume that HangingReceiveThread has been
// started and called Receive().
constexpr base::TimeDelta kThreadStartDelay = TimeDelta::FromMilliseconds(10);

class HangingReceiveThread : public DelegateSimpleThread::Delegate {
 public:
  explicit HangingReceiveThread(SyncSocket* socket, bool with_timeout)
      : socket_(socket),
        thread_(this, "HangingReceiveThread"),
        with_timeout_(with_timeout),
        done_event_(WaitableEvent::ResetPolicy::MANUAL,
                    WaitableEvent::InitialState::NOT_SIGNALED) {
    thread_.Start();
  }

  ~HangingReceiveThread() override {}

  void Run() override {
    int data = 0;
    ASSERT_EQ(socket_->Peek(), 0u);

    if (with_timeout_) {
      ASSERT_EQ(0u, socket_->ReceiveWithTimeout(&data, sizeof(data),
                                                kReceiveTimeout));
    } else {
      ASSERT_EQ(0u, socket_->Receive(&data, sizeof(data)));
    }

    done_event_.Signal();
  }

  void Stop() {
    thread_.Join();
  }

  WaitableEvent* done_event() { return &done_event_; }

 private:
  SyncSocket* socket_;
  DelegateSimpleThread thread_;
  bool with_timeout_;
  WaitableEvent done_event_;

  DISALLOW_COPY_AND_ASSIGN(HangingReceiveThread);
};

// Tests sending data between two SyncSockets. Uses ASSERT() and thus will exit
// early upon failure.  Callers should use ASSERT_NO_FATAL_FAILURE() if testing
// continues after return.
void SendReceivePeek(SyncSocket* socket_a, SyncSocket* socket_b) {
  int received = 0;
  const int kSending = 123;
  static_assert(sizeof(kSending) == sizeof(received), "invalid data size");

  ASSERT_EQ(0u, socket_a->Peek());
  ASSERT_EQ(0u, socket_b->Peek());

  // Verify |socket_a| can send to |socket_a| and |socket_a| can Receive from
  // |socket_a|.
  ASSERT_EQ(sizeof(kSending), socket_a->Send(&kSending, sizeof(kSending)));
  ASSERT_EQ(sizeof(kSending), socket_b->Peek());
  ASSERT_EQ(sizeof(kSending), socket_b->Receive(&received, sizeof(kSending)));
  ASSERT_EQ(kSending, received);

  ASSERT_EQ(0u, socket_a->Peek());
  ASSERT_EQ(0u, socket_b->Peek());

  // Now verify the reverse.
  received = 0;
  ASSERT_EQ(sizeof(kSending), socket_b->Send(&kSending, sizeof(kSending)));
  ASSERT_EQ(sizeof(kSending), socket_a->Peek());
  ASSERT_EQ(sizeof(kSending), socket_a->Receive(&received, sizeof(kSending)));
  ASSERT_EQ(kSending, received);

  ASSERT_EQ(0u, socket_a->Peek());
  ASSERT_EQ(0u, socket_b->Peek());

  ASSERT_TRUE(socket_a->Close());
  ASSERT_TRUE(socket_b->Close());
}

}  // namespace

class SyncSocketTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(SyncSocket::CreatePair(&socket_a_, &socket_b_));
  }

 protected:
  SyncSocket socket_a_;
  SyncSocket socket_b_;
};

TEST_F(SyncSocketTest, NormalSendReceivePeek) {
  SendReceivePeek(&socket_a_, &socket_b_);
}

TEST_F(SyncSocketTest, ClonedSendReceivePeek) {
  SyncSocket socket_c(socket_a_.Release());
  SyncSocket socket_d(socket_b_.Release());
  SendReceivePeek(&socket_c, &socket_d);
};

class CancelableSyncSocketTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(CancelableSyncSocket::CreatePair(&socket_a_, &socket_b_));
  }

 protected:
  CancelableSyncSocket socket_a_;
  CancelableSyncSocket socket_b_;
};

TEST_F(CancelableSyncSocketTest, NormalSendReceivePeek) {
  SendReceivePeek(&socket_a_, &socket_b_);
}

TEST_F(CancelableSyncSocketTest, ClonedSendReceivePeek) {
  CancelableSyncSocket socket_c(socket_a_.Release());
  CancelableSyncSocket socket_d(socket_b_.Release());
  SendReceivePeek(&socket_c, &socket_d);
}

TEST_F(CancelableSyncSocketTest, ShutdownCancelsReceive) {
  TimeTicks start = TimeTicks::Now();

  HangingReceiveThread thread(&socket_b_, true);

  PlatformThread::Sleep(kThreadStartDelay);
  ASSERT_TRUE(socket_b_.Shutdown());
  thread.done_event()->TimedWait(kReceiveTimeout);

  // Ensure the receive didn't just timeout.
  ASSERT_LT(TimeTicks::Now() - start, kReceiveTimeout);

  thread.Stop();
}

TEST_F(CancelableSyncSocketTest, ShutdownCancelsReceiveWithTimeout) {
  TimeTicks start = TimeTicks::Now();
  HangingReceiveThread thread(&socket_b_, true);

  PlatformThread::Sleep(kThreadStartDelay);
  ASSERT_TRUE(socket_b_.Shutdown());
  thread.done_event()->TimedWait(kReceiveTimeout);

  // Ensure the receive didn't just timeout.
  ASSERT_LT(TimeTicks::Now() - start, kReceiveTimeout);

  thread.Stop();
}

// Maximum amount of time Receive() is allowed to take when called after
// Shutdown().
constexpr base::TimeDelta kMaxNonblockingReceiveDelay =
    TimeDelta::FromMilliseconds(100);

TEST_F(CancelableSyncSocketTest, ReceiveAfterShutdown) {
  socket_a_.Shutdown();

  TimeTicks start = TimeTicks::Now();
  int data = 0;
  ASSERT_EQ(0u, socket_a_.Receive(&data, sizeof(data)));
  PlatformThread::Sleep(kThreadStartDelay);

  // Ensure the Receive() returned quickly.
  ASSERT_LT(TimeTicks::Now() - start, kMaxNonblockingReceiveDelay);
}

TEST_F(CancelableSyncSocketTest, ReceiveWithTimeoutAfterShutdown) {
  socket_a_.Shutdown();

  TimeTicks start = TimeTicks::Now();
  int data = 0;
  ASSERT_EQ(0u,
            socket_a_.ReceiveWithTimeout(&data, sizeof(data), kReceiveTimeout));
  PlatformThread::Sleep(kThreadStartDelay);

  // Ensure the ReceiveWithTimeout() returned quickly.
  ASSERT_LT(TimeTicks::Now() - start, kMaxNonblockingReceiveDelay);
}

}  // namespace base
