// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "mojo/edk/system/test_utils.h"
#include "mojo/edk/test/mojo_test_base.h"
#include "mojo/public/c/system/core.h"
#include "mojo/public/c/system/types.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {
namespace edk {
namespace {

const MojoHandleSignals kAllSignals = MOJO_HANDLE_SIGNAL_READABLE |
                                      MOJO_HANDLE_SIGNAL_WRITABLE |
                                      MOJO_HANDLE_SIGNAL_PEER_CLOSED;
static const char kHelloWorld[] = "hello world";

class MessagePipeTest : public test::MojoTestBase {
 public:
  MessagePipeTest() {
    CHECK_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &pipe0_, &pipe1_));
  }

  ~MessagePipeTest() override {
    if (pipe0_ != MOJO_HANDLE_INVALID)
      CHECK_EQ(MOJO_RESULT_OK, MojoClose(pipe0_));
    if (pipe1_ != MOJO_HANDLE_INVALID)
      CHECK_EQ(MOJO_RESULT_OK, MojoClose(pipe1_));
  }

  MojoResult WriteMessage(MojoHandle message_pipe_handle,
                          const void* bytes,
                          uint32_t num_bytes) {
    return mojo::WriteMessageRaw(MessagePipeHandle(message_pipe_handle), bytes,
                                 num_bytes, nullptr, 0,
                                 MOJO_WRITE_MESSAGE_FLAG_NONE);
  }

  MojoResult WriteMessageNew(MojoHandle message_pipe_handle,
                             MojoMessageHandle message_handle) {
    return MojoWriteMessageNew(message_pipe_handle, message_handle,
                               MOJO_WRITE_MESSAGE_FLAG_NONE);
  }

  MojoResult ReadMessage(MojoHandle message_pipe_handle,
                         void* bytes,
                         uint32_t* num_bytes,
                         bool may_discard = false) {
    MojoMessageHandle message_handle;
    MojoResult rv = MojoReadMessageNew(message_pipe_handle, &message_handle,
                                       MOJO_READ_MESSAGE_FLAG_NONE);
    if (rv != MOJO_RESULT_OK)
      return rv;

    const uint32_t expected_num_bytes = *num_bytes;
    void* buffer;
    rv = MojoGetSerializedMessageContents(
        message_handle, num_bytes, &buffer, nullptr, nullptr,
        MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE);

    if (rv == MOJO_RESULT_RESOURCE_EXHAUSTED) {
      CHECK(may_discard);
    } else if (*num_bytes) {
      CHECK_EQ(MOJO_RESULT_OK, rv);
      CHECK_GE(expected_num_bytes, *num_bytes);
      CHECK(bytes);
      memcpy(bytes, buffer, *num_bytes);
    }
    CHECK_EQ(MOJO_RESULT_OK, MojoFreeMessage(message_handle));
    return rv;
  }

  MojoResult ReadMessageNew(MojoHandle message_pipe_handle,
                            MojoMessageHandle* message_handle) {
    return MojoReadMessageNew(message_pipe_handle, message_handle,
                              MOJO_READ_MESSAGE_FLAG_NONE);
  }

  MojoHandle pipe0_, pipe1_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MessagePipeTest);
};

using FuseMessagePipeTest = test::MojoTestBase;

TEST_F(MessagePipeTest, WriteData) {
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteMessage(pipe0_, kHelloWorld, sizeof(kHelloWorld)));
}

// Tests:
//  - only default flags
//  - reading messages from a port
//    - when there are no/one/two messages available for that port
//    - with buffer size 0 (and null buffer) -- should get size
//    - with too-small buffer -- should get size
//    - also verify that buffers aren't modified when/where they shouldn't be
//  - writing messages to a port
//    - in the obvious scenarios (as above)
//    - to a port that's been closed
//  - writing a message to a port, closing the other (would be the source) port,
//    and reading it
TEST_F(MessagePipeTest, Basic) {
  int32_t buffer[2];
  const uint32_t kBufferSize = static_cast<uint32_t>(sizeof(buffer));
  uint32_t buffer_size;

  // Nothing to read yet on port 0.
  buffer[0] = 123;
  buffer[1] = 456;
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_SHOULD_WAIT, ReadMessage(pipe0_, buffer, &buffer_size));
  ASSERT_EQ(kBufferSize, buffer_size);
  ASSERT_EQ(123, buffer[0]);
  ASSERT_EQ(456, buffer[1]);

  // Ditto for port 1.
  buffer[0] = 123;
  buffer[1] = 456;
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_SHOULD_WAIT, ReadMessage(pipe1_, buffer, &buffer_size));

  // Write from port 1 (to port 0).
  buffer[0] = 789012345;
  buffer[1] = 0;
  ASSERT_EQ(MOJO_RESULT_OK, WriteMessage(pipe1_, buffer, sizeof(buffer[0])));

  MojoHandleSignalsState state;
  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe0_, MOJO_HANDLE_SIGNAL_READABLE, &state));

  // Read from port 0.
  buffer[0] = 123;
  buffer[1] = 456;
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_OK, ReadMessage(pipe0_, buffer, &buffer_size));
  ASSERT_EQ(static_cast<uint32_t>(sizeof(buffer[0])), buffer_size);
  ASSERT_EQ(789012345, buffer[0]);
  ASSERT_EQ(456, buffer[1]);

  // Read again from port 0 -- it should be empty.
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_SHOULD_WAIT, ReadMessage(pipe0_, buffer, &buffer_size));

  // Write two messages from port 0 (to port 1).
  buffer[0] = 123456789;
  buffer[1] = 0;
  ASSERT_EQ(MOJO_RESULT_OK, WriteMessage(pipe0_, buffer, sizeof(buffer[0])));
  buffer[0] = 234567890;
  buffer[1] = 0;
  ASSERT_EQ(MOJO_RESULT_OK, WriteMessage(pipe0_, buffer, sizeof(buffer[0])));

  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_READABLE, &state));

  // Read from port 1.
  buffer[0] = 123;
  buffer[1] = 456;
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_OK, ReadMessage(pipe1_, buffer, &buffer_size));
  ASSERT_EQ(static_cast<uint32_t>(sizeof(buffer[0])), buffer_size);
  ASSERT_EQ(123456789, buffer[0]);
  ASSERT_EQ(456, buffer[1]);

  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_READABLE, &state));

  // Read again from port 1.
  buffer[0] = 123;
  buffer[1] = 456;
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_OK, ReadMessage(pipe1_, buffer, &buffer_size));
  ASSERT_EQ(static_cast<uint32_t>(sizeof(buffer[0])), buffer_size);
  ASSERT_EQ(234567890, buffer[0]);
  ASSERT_EQ(456, buffer[1]);

  // Read again from port 1 -- it should be empty.
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_SHOULD_WAIT, ReadMessage(pipe1_, buffer, &buffer_size));

  // Write from port 0 (to port 1).
  buffer[0] = 345678901;
  buffer[1] = 0;
  ASSERT_EQ(MOJO_RESULT_OK, WriteMessage(pipe0_, buffer, sizeof(buffer[0])));

  // Close port 0.
  MojoClose(pipe0_);
  pipe0_ = MOJO_HANDLE_INVALID;

  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_PEER_CLOSED, &state));

  // Try to write from port 1 (to port 0).
  buffer[0] = 456789012;
  buffer[1] = 0;
  ASSERT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            WriteMessage(pipe1_, buffer, sizeof(buffer[0])));

  // Read from port 1; should still get message (even though port 0 was closed).
  buffer[0] = 123;
  buffer[1] = 456;
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_OK, ReadMessage(pipe1_, buffer, &buffer_size));
  ASSERT_EQ(static_cast<uint32_t>(sizeof(buffer[0])), buffer_size);
  ASSERT_EQ(345678901, buffer[0]);
  ASSERT_EQ(456, buffer[1]);

  // Read again from port 1 -- it should be empty (and port 0 is closed).
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            ReadMessage(pipe1_, buffer, &buffer_size));
}

TEST_F(MessagePipeTest, CloseWithQueuedIncomingMessages) {
  int32_t buffer[1];
  const uint32_t kBufferSize = static_cast<uint32_t>(sizeof(buffer));
  uint32_t buffer_size;

  // Write some messages from port 1 (to port 0).
  for (int32_t i = 0; i < 5; i++) {
    buffer[0] = i;
    ASSERT_EQ(MOJO_RESULT_OK, WriteMessage(pipe1_, buffer, kBufferSize));
  }

  MojoHandleSignalsState state;
  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe0_, MOJO_HANDLE_SIGNAL_READABLE, &state));

  // Port 0 shouldn't be empty.
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_OK, ReadMessage(pipe0_, buffer, &buffer_size));
  ASSERT_EQ(kBufferSize, buffer_size);

  // Close port 0 first, which should have outstanding (incoming) messages.
  MojoClose(pipe0_);
  MojoClose(pipe1_);
  pipe0_ = pipe1_ = MOJO_HANDLE_INVALID;
}

TEST_F(MessagePipeTest, BasicWaiting) {
  MojoHandleSignalsState hss;

  int32_t buffer[1];
  const uint32_t kBufferSize = static_cast<uint32_t>(sizeof(buffer));
  uint32_t buffer_size;

  // Always writable (until the other port is closed). Not yet readable. Peer
  // not closed.
  hss = GetSignalsState(pipe0_);
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, hss.satisfied_signals);
  ASSERT_EQ(kAllSignals, hss.satisfiable_signals);
  hss = MojoHandleSignalsState();

  // Write from port 0 (to port 1), to make port 1 readable.
  buffer[0] = 123456789;
  ASSERT_EQ(MOJO_RESULT_OK, WriteMessage(pipe0_, buffer, kBufferSize));

  // Port 1 should already be readable now.
  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_READABLE, &hss));
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            hss.satisfied_signals);
  ASSERT_EQ(kAllSignals, hss.satisfiable_signals);
  // ... and still writable.
  hss = MojoHandleSignalsState();
  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_WRITABLE, &hss));
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            hss.satisfied_signals);
  ASSERT_EQ(kAllSignals, hss.satisfiable_signals);

  // Close port 0.
  MojoClose(pipe0_);
  pipe0_ = MOJO_HANDLE_INVALID;

  // Port 1 should be signaled with peer closed.
  hss = MojoHandleSignalsState();
  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_PEER_CLOSED, &hss));
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfied_signals);
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfiable_signals);

  // Port 1 should not be writable.
  hss = MojoHandleSignalsState();

  ASSERT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_WRITABLE, &hss));
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfied_signals);
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfiable_signals);

  // But it should still be readable.
  hss = MojoHandleSignalsState();
  ASSERT_EQ(MOJO_RESULT_OK,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_READABLE, &hss));
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfied_signals);
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfiable_signals);

  // Read from port 1.
  buffer[0] = 0;
  buffer_size = kBufferSize;
  ASSERT_EQ(MOJO_RESULT_OK, ReadMessage(pipe1_, buffer, &buffer_size));
  ASSERT_EQ(123456789, buffer[0]);

  // Now port 1 should no longer be readable.
  hss = MojoHandleSignalsState();
  ASSERT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            WaitForSignals(pipe1_, MOJO_HANDLE_SIGNAL_READABLE, &hss));
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  ASSERT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);
}

TEST_F(MessagePipeTest, InvalidMessageObjects) {
  ASSERT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoFreeMessage(MOJO_MESSAGE_HANDLE_INVALID));

  ASSERT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoGetSerializedMessageContents(
                MOJO_MESSAGE_HANDLE_INVALID, nullptr, nullptr, nullptr, nullptr,
                MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE));

  ASSERT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoSerializeMessage(MOJO_MESSAGE_HANDLE_INVALID));

  MojoMessageHandle message_handle = MOJO_MESSAGE_HANDLE_INVALID;
  ASSERT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoCreateMessage(1, nullptr, nullptr));
  ASSERT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoCreateMessage(0, nullptr, &message_handle));
}

#if !defined(OS_IOS)

const size_t kPingPongHandlesPerIteration = 50;
const size_t kPingPongIterations = 500;

DEFINE_TEST_CLIENT_TEST_WITH_PIPE(HandlePingPong, MessagePipeTest, h) {
  // Waits for a handle to become readable and writes it back to the sender.
  for (size_t i = 0; i < kPingPongIterations; i++) {
    MojoHandle handles[kPingPongHandlesPerIteration];
    ReadMessageWithHandles(h, handles, kPingPongHandlesPerIteration);
    WriteMessageWithHandles(h, "", handles, kPingPongHandlesPerIteration);
  }

  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(h, MOJO_HANDLE_SIGNAL_READABLE));
  char msg[4];
  uint32_t num_bytes = 4;
  EXPECT_EQ(MOJO_RESULT_OK, ReadMessage(h, msg, &num_bytes));
}

// This test is flaky: http://crbug.com/585784
TEST_F(MessagePipeTest, DISABLED_DataPipeConsumerHandlePingPong) {
  MojoHandle p, c[kPingPongHandlesPerIteration];
  for (size_t i = 0; i < kPingPongHandlesPerIteration; ++i) {
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateDataPipe(nullptr, &p, &c[i]));
    MojoClose(p);
  }

  RUN_CHILD_ON_PIPE(HandlePingPong, h)
    for (size_t i = 0; i < kPingPongIterations; i++) {
      WriteMessageWithHandles(h, "", c, kPingPongHandlesPerIteration);
      ReadMessageWithHandles(h, c, kPingPongHandlesPerIteration);
    }
    WriteMessage(h, "quit", 4);
  END_CHILD()
  for (size_t i = 0; i < kPingPongHandlesPerIteration; ++i)
    MojoClose(c[i]);
}

// This test is flaky: http://crbug.com/585784
TEST_F(MessagePipeTest, DISABLED_DataPipeProducerHandlePingPong) {
  MojoHandle p[kPingPongHandlesPerIteration], c;
  for (size_t i = 0; i < kPingPongHandlesPerIteration; ++i) {
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateDataPipe(nullptr, &p[i], &c));
    MojoClose(c);
  }

  RUN_CHILD_ON_PIPE(HandlePingPong, h)
    for (size_t i = 0; i < kPingPongIterations; i++) {
      WriteMessageWithHandles(h, "", p, kPingPongHandlesPerIteration);
      ReadMessageWithHandles(h, p, kPingPongHandlesPerIteration);
    }
    WriteMessage(h, "quit", 4);
  END_CHILD()
  for (size_t i = 0; i < kPingPongHandlesPerIteration; ++i)
    MojoClose(p[i]);
}

TEST_F(MessagePipeTest, SharedBufferHandlePingPong) {
  MojoHandle buffers[kPingPongHandlesPerIteration];
  for (size_t i = 0; i <kPingPongHandlesPerIteration; ++i)
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateSharedBuffer(nullptr, 1, &buffers[i]));

  RUN_CHILD_ON_PIPE(HandlePingPong, h)
    for (size_t i = 0; i < kPingPongIterations; i++) {
      WriteMessageWithHandles(h, "", buffers, kPingPongHandlesPerIteration);
      ReadMessageWithHandles(h, buffers, kPingPongHandlesPerIteration);
    }
    WriteMessage(h, "quit", 4);
  END_CHILD()
  for (size_t i = 0; i < kPingPongHandlesPerIteration; ++i)
    MojoClose(buffers[i]);
}

#endif  // !defined(OS_IOS)

TEST_F(FuseMessagePipeTest, Basic) {
  // Test that we can fuse pipes and they still work.

  MojoHandle a, b, c, d;
  CreateMessagePipe(&a, &b);
  CreateMessagePipe(&c, &d);

  EXPECT_EQ(MOJO_RESULT_OK, MojoFuseMessagePipes(b, c));

  // Handles b and c should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(c));

  const std::string kTestMessage1 = "Hello, world!";
  const std::string kTestMessage2 = "Goodbye, world!";

  WriteMessage(a, kTestMessage1);
  EXPECT_EQ(kTestMessage1, ReadMessage(d));

  WriteMessage(d, kTestMessage2);
  EXPECT_EQ(kTestMessage2, ReadMessage(a));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(FuseMessagePipeTest, FuseAfterPeerWrite) {
  // Test that messages written before fusion are eventually delivered.

  MojoHandle a, b, c, d;
  CreateMessagePipe(&a, &b);
  CreateMessagePipe(&c, &d);

  const std::string kTestMessage1 = "Hello, world!";
  const std::string kTestMessage2 = "Goodbye, world!";
  WriteMessage(a, kTestMessage1);
  WriteMessage(d, kTestMessage2);

  EXPECT_EQ(MOJO_RESULT_OK, MojoFuseMessagePipes(b, c));

  // Handles b and c should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(c));

  EXPECT_EQ(kTestMessage1, ReadMessage(d));
  EXPECT_EQ(kTestMessage2, ReadMessage(a));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(FuseMessagePipeTest, NoFuseAfterWrite) {
  // Test that a pipe endpoint which has been written to cannot be fused.

  MojoHandle a, b, c, d;
  CreateMessagePipe(&a, &b);
  CreateMessagePipe(&c, &d);

  WriteMessage(b, "shouldn't have done that!");
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, MojoFuseMessagePipes(b, c));

  // Handles b and c should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(c));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(FuseMessagePipeTest, NoFuseSelf) {
  // Test that a pipe's own endpoints can't be fused together.

  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, MojoFuseMessagePipes(a, b));

  // Handles a and b should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(b));
}

TEST_F(FuseMessagePipeTest, FuseInvalidArguments) {
  MojoHandle a, b, c, d;
  CreateMessagePipe(&a, &b);
  CreateMessagePipe(&c, &d);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));

  // Can't fuse an invalid handle.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoFuseMessagePipes(b, c));

  // Handle c should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(c));

  // Can't fuse a non-message pipe handle.
  MojoHandle e, f;
  CreateDataPipe(&e, &f, 16);

  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoFuseMessagePipes(e, d));

  // Handles d and e should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(d));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(e));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(f));
}

TEST_F(FuseMessagePipeTest, FuseAfterPeerClosure) {
  // Test that peer closure prior to fusion can still be detected after fusion.

  MojoHandle a, b, c, d;
  CreateMessagePipe(&a, &b);
  CreateMessagePipe(&c, &d);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoFuseMessagePipes(b, c));

  // Handles b and c should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(c));

  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(d, MOJO_HANDLE_SIGNAL_PEER_CLOSED));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(FuseMessagePipeTest, FuseAfterPeerWriteAndClosure) {
  // Test that peer write and closure prior to fusion still results in the
  // both message arrival and awareness of peer closure.

  MojoHandle a, b, c, d;
  CreateMessagePipe(&a, &b);
  CreateMessagePipe(&c, &d);

  const std::string kTestMessage = "ayyy lmao";
  WriteMessage(a, kTestMessage);
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));

  EXPECT_EQ(MOJO_RESULT_OK, MojoFuseMessagePipes(b, c));

  // Handles b and c should be closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(c));

  EXPECT_EQ(kTestMessage, ReadMessage(d));
  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(d, MOJO_HANDLE_SIGNAL_PEER_CLOSED));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(MessagePipeTest, ClosePipesStressTest) {
  // Stress test to exercise https://crbug.com/665869.
  const size_t kNumPipes = 100000;
  for (size_t i = 0; i < kNumPipes; ++i) {
    MojoHandle a, b;
    CreateMessagePipe(&a, &b);
    MojoClose(a);
    MojoClose(b);
  }
}

// Helper class which provides a base implementation for an unserialized user
// message context and helpers to go between these objects and opaque message
// handles.
class TestMessageBase {
 public:
  virtual ~TestMessageBase() {}

  static MojoMessageHandle MakeMessageHandle(
      std::unique_ptr<TestMessageBase> message) {
    MojoMessageOperationThunks thunks;
    thunks.struct_size = sizeof(MojoMessageOperationThunks);
    thunks.get_serialized_size = &CallGetSerializedSize;
    thunks.serialize_handles = &CallSerializeHandles;
    thunks.serialize_payload = &CallSerializePayload;
    thunks.destroy = &Destroy;
    MojoMessageHandle handle;
    MojoResult rv = MojoCreateMessage(
        reinterpret_cast<uintptr_t>(message.release()), &thunks, &handle);
    DCHECK_EQ(MOJO_RESULT_OK, rv);
    return handle;
  }

  template <typename T>
  static std::unique_ptr<T> UnwrapMessageHandle(
      MojoMessageHandle* message_handle) {
    MojoMessageHandle handle = MOJO_HANDLE_INVALID;
    std::swap(handle, *message_handle);
    uintptr_t context;
    MojoResult rv = MojoReleaseMessageContext(handle, &context);
    DCHECK_EQ(MOJO_RESULT_OK, rv);
    MojoFreeMessage(handle);
    return base::WrapUnique(reinterpret_cast<T*>(context));
  }

 protected:
  virtual void GetSerializedSize(size_t* num_bytes, size_t* num_handles) = 0;
  virtual void SerializeHandles(MojoHandle* handles) = 0;
  virtual void SerializePayload(void* buffer) = 0;

 private:
  static void CallGetSerializedSize(uintptr_t context,
                                    size_t* num_bytes,
                                    size_t* num_handles) {
    reinterpret_cast<TestMessageBase*>(context)->GetSerializedSize(num_bytes,
                                                                   num_handles);
  }

  static void CallSerializeHandles(uintptr_t context, MojoHandle* handles) {
    reinterpret_cast<TestMessageBase*>(context)->SerializeHandles(handles);
  }

  static void CallSerializePayload(uintptr_t context, void* buffer) {
    reinterpret_cast<TestMessageBase*>(context)->SerializePayload(buffer);
  }

  static void Destroy(uintptr_t context) {
    delete reinterpret_cast<TestMessageBase*>(context);
  }
};

class NeverSerializedMessage : public TestMessageBase {
 public:
  NeverSerializedMessage(
      const base::Closure& destruction_callback = base::Closure())
      : destruction_callback_(destruction_callback) {}
  ~NeverSerializedMessage() override {
    if (destruction_callback_)
      destruction_callback_.Run();
  }

 private:
  // TestMessageBase:
  void GetSerializedSize(size_t* num_bytes, size_t* num_handles) override {
    NOTREACHED();
  }
  void SerializeHandles(MojoHandle* handles) override { NOTREACHED(); }
  void SerializePayload(void* buffer) override { NOTREACHED(); }

  const base::Closure destruction_callback_;

  DISALLOW_COPY_AND_ASSIGN(NeverSerializedMessage);
};

TEST_F(MessagePipeTest, SendLocalMessageWithContext) {
  // Simple write+read of a message with context. Verifies that such messages
  // are passed through a local pipe without serialization.
  auto message = base::MakeUnique<NeverSerializedMessage>();
  auto* original_message = message.get();

  MojoHandle a, b;
  CreateMessagePipe(&a, &b);
  EXPECT_EQ(MOJO_RESULT_OK,
            WriteMessageNew(
                a, TestMessageBase::MakeMessageHandle(std::move(message))));
  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(b, MOJO_HANDLE_SIGNAL_READABLE));

  MojoMessageHandle read_message_handle;
  EXPECT_EQ(MOJO_RESULT_OK, ReadMessageNew(b, &read_message_handle));
  message = TestMessageBase::UnwrapMessageHandle<NeverSerializedMessage>(
      &read_message_handle);
  EXPECT_EQ(original_message, message.get());

  MojoClose(a);
  MojoClose(b);
}

TEST_F(MessagePipeTest, FreeMessageWithContext) {
  // Tests that |MojoFreeMessage()| destroys any attached context.
  bool was_deleted = false;
  auto message = base::MakeUnique<NeverSerializedMessage>(
      base::Bind([](bool* was_deleted) { *was_deleted = true; }, &was_deleted));
  MojoMessageHandle handle =
      TestMessageBase::MakeMessageHandle(std::move(message));
  EXPECT_FALSE(was_deleted);
  EXPECT_EQ(MOJO_RESULT_OK, MojoFreeMessage(handle));
  EXPECT_TRUE(was_deleted);
}

class SimpleMessage : public TestMessageBase {
 public:
  SimpleMessage(const std::string& contents,
                const base::Closure& destruction_callback = base::Closure())
      : contents_(contents), destruction_callback_(destruction_callback) {}

  ~SimpleMessage() override {
    if (destruction_callback_)
      destruction_callback_.Run();
  }

  void AddMessagePipe(mojo::ScopedMessagePipeHandle handle) {
    handles_.emplace_back(std::move(handle));
  }

  std::vector<mojo::ScopedMessagePipeHandle>& handles() { return handles_; }

 private:
  // TestMessageBase:
  void GetSerializedSize(size_t* num_bytes, size_t* num_handles) override {
    *num_bytes = contents_.size();
    *num_handles = handles_.size();
  }

  void SerializeHandles(MojoHandle* handles) override {
    ASSERT_TRUE(!handles_.empty());
    for (size_t i = 0; i < handles_.size(); ++i)
      handles[i] = handles_[i].release().value();
    handles_.clear();
  }

  void SerializePayload(void* buffer) override {
    std::copy(contents_.begin(), contents_.end(), static_cast<char*>(buffer));
  }

  const std::string contents_;
  const base::Closure destruction_callback_;
  std::vector<mojo::ScopedMessagePipeHandle> handles_;

  DISALLOW_COPY_AND_ASSIGN(SimpleMessage);
};

const char kTestMessageWithContext1[] = "hello laziness";

#if !defined(OS_IOS)

const char kTestMessageWithContext2[] = "my old friend";
const char kTestMessageWithContext3[] = "something something";
const char kTestMessageWithContext4[] = "do moar ipc";

DEFINE_TEST_CLIENT_TEST_WITH_PIPE(ReceiveMessageNoHandles, MessagePipeTest, h) {
  MojoTestBase::WaitForSignals(h, MOJO_HANDLE_SIGNAL_READABLE);
  auto m = MojoTestBase::ReadMessage(h);
  EXPECT_EQ(kTestMessageWithContext1, m);
}

TEST_F(MessagePipeTest, SerializeSimpleMessageNoHandlesWithContext) {
  RUN_CHILD_ON_PIPE(ReceiveMessageNoHandles, h)
    auto message = base::MakeUnique<SimpleMessage>(kTestMessageWithContext1);
    WriteMessageNew(h, TestMessageBase::MakeMessageHandle(std::move(message)));
  END_CHILD()
}

DEFINE_TEST_CLIENT_TEST_WITH_PIPE(ReceiveMessageOneHandle, MessagePipeTest, h) {
  MojoTestBase::WaitForSignals(h, MOJO_HANDLE_SIGNAL_READABLE);
  MojoHandle h1;
  auto m = MojoTestBase::ReadMessageWithHandles(h, &h1, 1);
  EXPECT_EQ(kTestMessageWithContext1, m);
  MojoTestBase::WriteMessage(h1, kTestMessageWithContext2);
}

TEST_F(MessagePipeTest, SerializeSimpleMessageOneHandleWithContext) {
  RUN_CHILD_ON_PIPE(ReceiveMessageOneHandle, h)
    auto message = base::MakeUnique<SimpleMessage>(kTestMessageWithContext1);
    mojo::MessagePipe pipe;
    message->AddMessagePipe(std::move(pipe.handle0));
    WriteMessageNew(h, TestMessageBase::MakeMessageHandle(std::move(message)));
    EXPECT_EQ(kTestMessageWithContext2,
              MojoTestBase::ReadMessage(pipe.handle1.get().value()));
  END_CHILD()
}

DEFINE_TEST_CLIENT_TEST_WITH_PIPE(ReceiveMessageWithHandles,
                                  MessagePipeTest,
                                  h) {
  MojoTestBase::WaitForSignals(h, MOJO_HANDLE_SIGNAL_READABLE);
  MojoHandle handles[4];
  auto m = MojoTestBase::ReadMessageWithHandles(h, handles, 4);
  EXPECT_EQ(kTestMessageWithContext1, m);
  MojoTestBase::WriteMessage(handles[0], kTestMessageWithContext1);
  MojoTestBase::WriteMessage(handles[1], kTestMessageWithContext2);
  MojoTestBase::WriteMessage(handles[2], kTestMessageWithContext3);
  MojoTestBase::WriteMessage(handles[3], kTestMessageWithContext4);
}

TEST_F(MessagePipeTest, SerializeSimpleMessageWithHandlesWithContext) {
  RUN_CHILD_ON_PIPE(ReceiveMessageWithHandles, h)
    auto message = base::MakeUnique<SimpleMessage>(kTestMessageWithContext1);
    mojo::MessagePipe pipes[4];
    message->AddMessagePipe(std::move(pipes[0].handle0));
    message->AddMessagePipe(std::move(pipes[1].handle0));
    message->AddMessagePipe(std::move(pipes[2].handle0));
    message->AddMessagePipe(std::move(pipes[3].handle0));
    WriteMessageNew(h, TestMessageBase::MakeMessageHandle(std::move(message)));
    EXPECT_EQ(kTestMessageWithContext1,
              MojoTestBase::ReadMessage(pipes[0].handle1.get().value()));
    EXPECT_EQ(kTestMessageWithContext2,
              MojoTestBase::ReadMessage(pipes[1].handle1.get().value()));
    EXPECT_EQ(kTestMessageWithContext3,
              MojoTestBase::ReadMessage(pipes[2].handle1.get().value()));
    EXPECT_EQ(kTestMessageWithContext4,
              MojoTestBase::ReadMessage(pipes[3].handle1.get().value()));
  END_CHILD()
}

#endif  // !defined(OS_IOS)

TEST_F(MessagePipeTest, SendLocalSimpleMessageWithHandlesWithContext) {
  auto message = base::MakeUnique<SimpleMessage>(kTestMessageWithContext1);
  auto* original_message = message.get();
  mojo::MessagePipe pipes[4];
  MojoHandle original_handles[4] = {
      pipes[0].handle0.get().value(), pipes[1].handle0.get().value(),
      pipes[2].handle0.get().value(), pipes[3].handle0.get().value(),
  };
  message->AddMessagePipe(std::move(pipes[0].handle0));
  message->AddMessagePipe(std::move(pipes[1].handle0));
  message->AddMessagePipe(std::move(pipes[2].handle0));
  message->AddMessagePipe(std::move(pipes[3].handle0));

  MojoHandle a, b;
  CreateMessagePipe(&a, &b);
  EXPECT_EQ(MOJO_RESULT_OK,
            WriteMessageNew(
                a, TestMessageBase::MakeMessageHandle(std::move(message))));
  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(b, MOJO_HANDLE_SIGNAL_READABLE));

  MojoMessageHandle read_message_handle;
  EXPECT_EQ(MOJO_RESULT_OK, ReadMessageNew(b, &read_message_handle));
  message =
      TestMessageBase::UnwrapMessageHandle<SimpleMessage>(&read_message_handle);
  EXPECT_EQ(original_message, message.get());
  ASSERT_EQ(4u, message->handles().size());
  EXPECT_EQ(original_handles[0], message->handles()[0].get().value());
  EXPECT_EQ(original_handles[1], message->handles()[1].get().value());
  EXPECT_EQ(original_handles[2], message->handles()[2].get().value());
  EXPECT_EQ(original_handles[3], message->handles()[3].get().value());

  MojoClose(a);
  MojoClose(b);
}

TEST_F(MessagePipeTest, DropUnreadLocalMessageWithContext) {
  // Verifies that if a message is sent with context over a pipe and the
  // receiver closes without reading the message, the context is properly
  // cleaned up.
  bool message_was_destroyed = false;
  auto message = base::MakeUnique<SimpleMessage>(
      kTestMessageWithContext1,
      base::Bind([](bool* was_destroyed) { *was_destroyed = true; },
                 &message_was_destroyed));

  mojo::MessagePipe pipe;
  message->AddMessagePipe(std::move(pipe.handle0));
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);
  EXPECT_EQ(MOJO_RESULT_OK,
            WriteMessageNew(
                a, TestMessageBase::MakeMessageHandle(std::move(message))));
  MojoClose(a);
  MojoClose(b);

  EXPECT_TRUE(message_was_destroyed);
  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(pipe.handle1.get().value(),
                                           MOJO_HANDLE_SIGNAL_PEER_CLOSED));
}

TEST_F(MessagePipeTest, ReadMessageWithContextAsSerializedMessage) {
  bool message_was_destroyed = false;
  std::unique_ptr<TestMessageBase> message =
      base::MakeUnique<NeverSerializedMessage>(
          base::Bind([](bool* was_destroyed) { *was_destroyed = true; },
                     &message_was_destroyed));

  MojoHandle a, b;
  CreateMessagePipe(&a, &b);
  EXPECT_EQ(MOJO_RESULT_OK,
            WriteMessageNew(
                a, TestMessageBase::MakeMessageHandle(std::move(message))));
  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(b, MOJO_HANDLE_SIGNAL_READABLE));

  MojoMessageHandle message_handle;
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadMessageNew(b, &message_handle,
                                               MOJO_READ_MESSAGE_FLAG_NONE));
  EXPECT_FALSE(message_was_destroyed);

  // Not a serialized message, so we can't get serialized contents.
  uint32_t num_bytes;
  void* buffer;
  uint32_t num_handles = 0;
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoGetSerializedMessageContents(
                message_handle, &num_bytes, &buffer, &num_handles, nullptr,
                MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE));
  EXPECT_FALSE(message_was_destroyed);

  EXPECT_EQ(MOJO_RESULT_OK, MojoFreeMessage(message_handle));
  EXPECT_TRUE(message_was_destroyed);

  MojoClose(a);
  MojoClose(b);
}

TEST_F(MessagePipeTest, ReadSerializedMessageAsMessageWithContext) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);
  MojoTestBase::WriteMessage(a, "hello there");
  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(b, MOJO_HANDLE_SIGNAL_READABLE));

  MojoMessageHandle message_handle;
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadMessageNew(b, &message_handle,
                                               MOJO_READ_MESSAGE_FLAG_NONE));
  uintptr_t context;
  EXPECT_EQ(MOJO_RESULT_NOT_FOUND,
            MojoReleaseMessageContext(message_handle, &context));
  MojoClose(a);
  MojoClose(b);
}

TEST_F(MessagePipeTest, ForceSerializeMessageWithContext) {
  // Basic test - we can serialize a simple message.
  bool message_was_destroyed = false;
  auto message = base::MakeUnique<SimpleMessage>(
      kTestMessageWithContext1,
      base::Bind([](bool* was_destroyed) { *was_destroyed = true; },
                 &message_was_destroyed));
  auto message_handle = TestMessageBase::MakeMessageHandle(std::move(message));
  EXPECT_EQ(MOJO_RESULT_OK, MojoSerializeMessage(message_handle));
  EXPECT_TRUE(message_was_destroyed);
  EXPECT_EQ(MOJO_RESULT_OK, MojoFreeMessage(message_handle));

  // Serialize a message with a single handle. Freeing the message should close
  // the handle.
  message_was_destroyed = false;
  message = base::MakeUnique<SimpleMessage>(
      kTestMessageWithContext1,
      base::Bind([](bool* was_destroyed) { *was_destroyed = true; },
                 &message_was_destroyed));
  MessagePipe pipe1;
  message->AddMessagePipe(std::move(pipe1.handle0));
  message_handle = TestMessageBase::MakeMessageHandle(std::move(message));
  EXPECT_EQ(MOJO_RESULT_OK, MojoSerializeMessage(message_handle));
  EXPECT_TRUE(message_was_destroyed);
  EXPECT_EQ(MOJO_RESULT_OK, MojoFreeMessage(message_handle));
  EXPECT_EQ(MOJO_RESULT_OK, WaitForSignals(pipe1.handle1.get().value(),
                                           MOJO_HANDLE_SIGNAL_PEER_CLOSED));

  // Serialize a message with a handle and extract its serialized contents.
  message_was_destroyed = false;
  message = base::MakeUnique<SimpleMessage>(
      kTestMessageWithContext1,
      base::Bind([](bool* was_destroyed) { *was_destroyed = true; },
                 &message_was_destroyed));
  MessagePipe pipe2;
  message->AddMessagePipe(std::move(pipe2.handle0));
  message_handle = TestMessageBase::MakeMessageHandle(std::move(message));
  EXPECT_EQ(MOJO_RESULT_OK, MojoSerializeMessage(message_handle));
  EXPECT_TRUE(message_was_destroyed);
  uint32_t num_bytes = 0;
  void* buffer = nullptr;
  uint32_t num_handles = 0;
  MojoHandle extracted_handle;
  EXPECT_EQ(MOJO_RESULT_RESOURCE_EXHAUSTED,
            MojoGetSerializedMessageContents(
                message_handle, &num_bytes, &buffer, &num_handles, nullptr,
                MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetSerializedMessageContents(
                message_handle, &num_bytes, &buffer, nullptr, nullptr,
                MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_IGNORE_HANDLES));
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoGetSerializedMessageContents(
          message_handle, &num_bytes, &buffer, &num_handles, &extracted_handle,
          MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE));
  EXPECT_EQ(std::string(kTestMessageWithContext1).size(), num_bytes);
  EXPECT_EQ(std::string(kTestMessageWithContext1),
            base::StringPiece(static_cast<char*>(buffer), num_bytes));

  // Confirm that the handle we extracted from the serialized message is still
  // connected to the same peer, despite the fact that its handle value may have
  // changed.
  const char kTestMessage[] = "hey you";
  MojoTestBase::WriteMessage(pipe2.handle1.get().value(), kTestMessage);
  EXPECT_EQ(MOJO_RESULT_OK,
            WaitForSignals(extracted_handle, MOJO_HANDLE_SIGNAL_READABLE));
  EXPECT_EQ(kTestMessage, MojoTestBase::ReadMessage(extracted_handle));

  EXPECT_EQ(MOJO_RESULT_OK, MojoFreeMessage(message_handle));
}

}  // namespace
}  // namespace edk
}  // namespace mojo
