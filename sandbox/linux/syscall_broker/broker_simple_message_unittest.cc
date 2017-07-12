// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/syscall_broker/broker_simple_message.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "sandbox/linux/syscall_broker/broker_channel.h"
#include "sandbox/linux/syscall_broker/broker_simple_message.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {

namespace syscall_broker {

namespace {

void PostWaitableEventToThread(base::Thread* thread,
                               base::WaitableEvent* wait_event) {
  thread->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(wait_event)));
}

}  // namespace

class ExpectedResultValue {
 public:
  virtual bool NextMessagePieceMatches(BrokerSimpleMessage* message) = 0;
  virtual int Size() = 0;
};

class ExpectedResultDataValue : public ExpectedResultValue {
 public:
  ExpectedResultDataValue(const char* data, int length);

  bool NextMessagePieceMatches(BrokerSimpleMessage* message) override;
  int Size() override;

 private:
  const char* data_;
  int length_;
  ExpectedResultDataValue();
};

class ExpectedResultIntValue : public ExpectedResultValue {
 public:
  ExpectedResultIntValue(int value);

  bool NextMessagePieceMatches(BrokerSimpleMessage* message) override;
  int Size() override;

 private:
  int value_;

  ExpectedResultIntValue();
};

class BrokerSimpleMessageTestHelper {
 public:
  static bool MessageContentMatches(const BrokerSimpleMessage& message,
                                    const uint8_t* content,
                                    size_t length);

  static void SendMsg(int write_fd, BrokerSimpleMessage* message, int fd);

  static void RecvMsg(BrokerChannel::EndPoint* ipc_reader,
                      ExpectedResultValue** expected_values,
                      int expected_values_length);

  static void RecvMsgAndReply(BrokerChannel::EndPoint* ipc_reader,
                              ExpectedResultValue** expected_values,
                              int expected_values_length,
                              const char* response_msg,
                              int fd);

  // Helper functions to write the respective BrokerSimpleMessage::EntryType to
  // a buffer. Returns the pointer to the memory right after where the value
  // was written to in |dst|.
  static uint8_t* WriteDataType(uint8_t* dst);
  static uint8_t* WriteIntType(uint8_t* dst);

  static size_t entry_type_size() {
    return sizeof(BrokerSimpleMessage::EntryType);
  }
};

ExpectedResultDataValue::ExpectedResultDataValue() {}

ExpectedResultDataValue::ExpectedResultDataValue(const char* data, int length)
    : data_(data), length_(length) {}

bool ExpectedResultDataValue::NextMessagePieceMatches(
    BrokerSimpleMessage* message) {
  const char* next_data;
  int next_length;

  if (!message->ReadData(&next_data, &next_length))
    return false;

  if (next_length != length_)
    return false;

  return strncmp(data_, next_data, length_) == 0;
}

int ExpectedResultDataValue::Size() {
  return sizeof(int) + length_ * sizeof(char) +
         BrokerSimpleMessageTestHelper::entry_type_size();
}

ExpectedResultIntValue::ExpectedResultIntValue() {}

ExpectedResultIntValue::ExpectedResultIntValue(int value) : value_(value) {}

bool ExpectedResultIntValue::NextMessagePieceMatches(
    BrokerSimpleMessage* message) {
  int next_value;

  if (!message->ReadInt(&next_value))
    return false;

  return next_value == value_;
}

int ExpectedResultIntValue::Size() {
  return sizeof(int) + BrokerSimpleMessageTestHelper::entry_type_size();
}

// static
bool BrokerSimpleMessageTestHelper::MessageContentMatches(
    const BrokerSimpleMessage& message,
    const uint8_t* content,
    size_t length) {
  return length == message.length_ &&
         memcmp(message.message_, content, length) == 0;
}

// static
void BrokerSimpleMessageTestHelper::SendMsg(int write_fd,
                                            BrokerSimpleMessage* message,
                                            int fd) {
  EXPECT_NE(-1, BrokerSimpleMessage::SendMsg(write_fd, *message, fd));
}

// static
void BrokerSimpleMessageTestHelper::RecvMsg(
    BrokerChannel::EndPoint* ipc_reader,
    ExpectedResultValue** expected_values,
    int expected_values_length) {
  base::ScopedFD return_fd;
  BrokerSimpleMessage message;
  ssize_t len = BrokerSimpleMessage::RecvMsgWithFlags(ipc_reader->get(),
                                                      &message, 0, &return_fd);

  EXPECT_LT(0, len) << "foobar";

  size_t expected_message_size = 0;
  for (int i = 0; i < expected_values_length; i++) {
    ExpectedResultValue* expected_result = expected_values[i];
    EXPECT_TRUE(expected_result->NextMessagePieceMatches(&message));
    expected_message_size += expected_result->Size();
  }

  EXPECT_EQ(expected_message_size, static_cast<size_t>(len));
}

// static
void BrokerSimpleMessageTestHelper::RecvMsgAndReply(
    BrokerChannel::EndPoint* ipc_reader,
    ExpectedResultValue** expected_values,
    int expected_values_length,
    const char* response_msg,
    int fd) {
  base::ScopedFD return_fd;
  BrokerSimpleMessage message;
  ssize_t len = BrokerSimpleMessage::RecvMsgWithFlags(ipc_reader->get(),
                                                      &message, 0, &return_fd);

  EXPECT_LT(0, len);

  size_t expected_message_size = 0;
  for (int i = 0; i < expected_values_length; i++) {
    ExpectedResultValue* expected_result = expected_values[i];
    EXPECT_TRUE(expected_result->NextMessagePieceMatches(&message));
    expected_message_size += expected_result->Size();
  }

  EXPECT_EQ(expected_message_size, static_cast<size_t>(len));

  BrokerSimpleMessage response_message;
  response_message.AddDataToMessage(response_msg, strlen(response_msg) + 1);
  SendMsg(return_fd.get(), &response_message, -1);
}

// static
uint8_t* BrokerSimpleMessageTestHelper::WriteDataType(uint8_t* dst) {
  BrokerSimpleMessage::EntryType type = BrokerSimpleMessage::EntryType::DATA;
  memcpy(dst, &type, sizeof(BrokerSimpleMessage::EntryType));
  return dst + sizeof(BrokerSimpleMessage::EntryType);
}

// static
uint8_t* BrokerSimpleMessageTestHelper::WriteIntType(uint8_t* dst) {
  BrokerSimpleMessage::EntryType type = BrokerSimpleMessage::EntryType::INT;
  memcpy(dst, &type, sizeof(BrokerSimpleMessage::EntryType));
  return dst + sizeof(BrokerSimpleMessage::EntryType);
}

TEST(BrokerSimpleMessage, AddData) {
  const char data1[] = "hello, world";
  const char data2[] = "foobar";
  const int int1 = 42;
  const int int2 = 24;
  uint8_t message_content[kMaxMessageLength];
  uint8_t* next;
  int len;

  // Simple string
  {
    BrokerSimpleMessage message;
    message.AddDataToMessage(data1, strlen(data1));

    next = BrokerSimpleMessageTestHelper::WriteDataType(message_content);
    len = static_cast<int>(strlen(data1));
    memcpy(next, &len, sizeof(int));
    next = next + sizeof(int);
    memcpy(next, data1, strlen(data1));
    next = next + strlen(data1);

    EXPECT_TRUE(BrokerSimpleMessageTestHelper::MessageContentMatches(
        message, message_content, next - message_content));
  }

  // Simple int
  {
    BrokerSimpleMessage message;
    message.AddIntToMessage(int1);

    next = BrokerSimpleMessageTestHelper::WriteIntType(message_content);
    memcpy(next, &int1, sizeof(int));
    next = next + sizeof(int);

    EXPECT_TRUE(BrokerSimpleMessageTestHelper::MessageContentMatches(
        message, message_content, next - message_content));
  }

  // string then int
  {
    BrokerSimpleMessage message;
    message.AddDataToMessage(data1, strlen(data1));
    message.AddIntToMessage(int1);

    // string
    next = BrokerSimpleMessageTestHelper::WriteDataType(message_content);
    len = static_cast<int>(strlen(data1));
    memcpy(next, &len, sizeof(int));
    next = next + sizeof(int);
    memcpy(next, data1, strlen(data1));
    next = next + strlen(data1);

    // int
    next = BrokerSimpleMessageTestHelper::WriteIntType(next);
    memcpy(next, &int1, sizeof(int));
    next = next + sizeof(int);

    EXPECT_TRUE(BrokerSimpleMessageTestHelper::MessageContentMatches(
        message, message_content, next - message_content));
  }

  // int then string
  {
    BrokerSimpleMessage message;
    message.AddIntToMessage(int1);
    message.AddDataToMessage(data1, strlen(data1));

    // int
    next = BrokerSimpleMessageTestHelper::WriteIntType(message_content);
    memcpy(next, &int1, sizeof(int));
    next = next + sizeof(int);

    // string
    next = BrokerSimpleMessageTestHelper::WriteDataType(next);
    len = static_cast<int>(strlen(data1));
    memcpy(next, &len, sizeof(int));
    next = next + sizeof(int);
    memcpy(next, data1, strlen(data1));
    next = next + strlen(data1);

    EXPECT_TRUE(BrokerSimpleMessageTestHelper::MessageContentMatches(
        message, message_content, next - message_content));
  }

  // string int string int
  {
    BrokerSimpleMessage message;
    message.AddDataToMessage(data1, strlen(data1));
    message.AddIntToMessage(int1);
    message.AddDataToMessage(data2, strlen(data2));
    message.AddIntToMessage(int2);

    // string
    next = BrokerSimpleMessageTestHelper::WriteDataType(message_content);
    len = static_cast<int>(strlen(data1));
    memcpy(next, &len, sizeof(int));
    next = next + sizeof(int);
    memcpy(next, data1, strlen(data1));
    next = next + strlen(data1);

    // int
    next = BrokerSimpleMessageTestHelper::WriteIntType(next);
    memcpy(next, &int1, sizeof(int));
    next = next + sizeof(int);

    // string
    next = BrokerSimpleMessageTestHelper::WriteDataType(next);
    len = static_cast<int>(strlen(data2));
    memcpy(next, &len, sizeof(int));
    next = next + sizeof(int);
    memcpy(next, data2, strlen(data2));
    next = next + strlen(data2);

    // int
    next = BrokerSimpleMessageTestHelper::WriteIntType(next);
    memcpy(next, &int2, sizeof(int));
    next = next + sizeof(int);

    EXPECT_TRUE(BrokerSimpleMessageTestHelper::MessageContentMatches(
        message, message_content, next - message_content));
  }
}

TEST(BrokerSimpleMessage, SendAndRecvMsg) {
  const char data1[] = "hello, world";
  const char data2[] = "foobar";
  const int int1 = 42;
  const int int2 = 24;

  base::Thread message_thread("SendMessageThread");
  ASSERT_TRUE(message_thread.Start());
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED);

  // Simple string case
  {
    SCOPED_TRACE("Simple string case");
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    BrokerSimpleMessage send_message;
    send_message.AddDataToMessage(data1, strlen(data1) + 1);
    message_thread.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&BrokerSimpleMessageTestHelper::SendMsg,
                                  ipc_writer.get(), &send_message, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    ExpectedResultDataValue data1_value(data1, strlen(data1) + 1);
    ExpectedResultValue* expected_results[] = {&data1_value};

    BrokerSimpleMessageTestHelper::RecvMsg(&ipc_reader, expected_results,
                                           arraysize(expected_results));

    wait_event.Wait();
  }

  // Simple int case
  {
    SCOPED_TRACE("Simple int case");
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    BrokerSimpleMessage send_message;
    send_message.AddIntToMessage(int1);
    message_thread.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&BrokerSimpleMessageTestHelper::SendMsg,
                                  ipc_writer.get(), &send_message, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    ExpectedResultIntValue int1_value(int1);
    ExpectedResultValue* expected_results[] = {&int1_value};

    BrokerSimpleMessageTestHelper::RecvMsg(&ipc_reader, expected_results,
                                           arraysize(expected_results));

    wait_event.Wait();
  }

  // Mixed message 1
  {
    SCOPED_TRACE("Mixed message 1");
    base::Thread message_thread("SendMessageThread");
    ASSERT_TRUE(message_thread.Start());
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    BrokerSimpleMessage send_message;
    send_message.AddDataToMessage(data1, strlen(data1) + 1);
    send_message.AddIntToMessage(int1);
    message_thread.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&BrokerSimpleMessageTestHelper::SendMsg,
                                  ipc_writer.get(), &send_message, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    ExpectedResultDataValue data1_value(data1, strlen(data1) + 1);
    ExpectedResultIntValue int1_value(int1);
    ExpectedResultValue* expected_results[] = {&data1_value, &int1_value};

    BrokerSimpleMessageTestHelper::RecvMsg(&ipc_reader, expected_results,
                                           arraysize(expected_results));

    wait_event.Wait();
  }

  // Mixed message 2
  {
    SCOPED_TRACE("Mixed message 2");
    base::Thread message_thread("SendMessageThread");
    ASSERT_TRUE(message_thread.Start());
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    BrokerSimpleMessage send_message;
    send_message.AddIntToMessage(int1);
    send_message.AddDataToMessage(data1, strlen(data1) + 1);
    send_message.AddDataToMessage(data2, strlen(data2) + 1);
    send_message.AddIntToMessage(int2);
    message_thread.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&BrokerSimpleMessageTestHelper::SendMsg,
                                  ipc_writer.get(), &send_message, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    ExpectedResultDataValue data1_value(data1, strlen(data1) + 1);
    ExpectedResultDataValue data2_value(data2, strlen(data2) + 1);
    ExpectedResultIntValue int1_value(int1);
    ExpectedResultIntValue int2_value(int2);
    ExpectedResultValue* expected_results[] = {&int1_value, &data1_value,
                                               &data2_value, &int2_value};

    BrokerSimpleMessageTestHelper::RecvMsg(&ipc_reader, expected_results,
                                           arraysize(expected_results));

    wait_event.Wait();
  }
}

TEST(BrokerSimpleMessage, SendRecvMsgSynchronous) {
  const char data1[] = "hello, world";
  const char data2[] = "foobar";
  const int int1 = 42;
  const int int2 = 24;
  const char reply_data1[] = "baz";

  base::Thread message_thread("SendMessageThread");
  ASSERT_TRUE(message_thread.Start());
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED);

  // Simple string case
  {
    SCOPED_TRACE("Simple string case");
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    ExpectedResultDataValue data1_value(data1, strlen(data1) + 1);
    ExpectedResultValue* expected_results[] = {&data1_value};
    message_thread.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&BrokerSimpleMessageTestHelper::RecvMsgAndReply,
                       &ipc_reader, expected_results,
                       arraysize(expected_results), reply_data1, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    BrokerSimpleMessage send_message;
    send_message.AddDataToMessage(data1, strlen(data1) + 1);
    BrokerSimpleMessage reply_message;
    int returned_fd;
    ssize_t len = BrokerSimpleMessage::SendRecvMsgWithFlags(
        ipc_writer.get(), &reply_message, 0, &returned_fd, send_message);

    ExpectedResultDataValue response_value(reply_data1,
                                           strlen(reply_data1) + 1);

    EXPECT_TRUE(response_value.NextMessagePieceMatches(&reply_message));
    EXPECT_EQ(len, response_value.Size());

    wait_event.Wait();
  }

  // Simple int case
  {
    SCOPED_TRACE("Simple int case");
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    ExpectedResultIntValue int1_value(int1);
    ExpectedResultValue* expected_results[] = {&int1_value};
    message_thread.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&BrokerSimpleMessageTestHelper::RecvMsgAndReply,
                       &ipc_reader, expected_results,
                       arraysize(expected_results), reply_data1, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    BrokerSimpleMessage send_message;
    send_message.AddIntToMessage(int1);
    BrokerSimpleMessage reply_message;
    int returned_fd;
    ssize_t len = BrokerSimpleMessage::SendRecvMsgWithFlags(
        ipc_writer.get(), &reply_message, 0, &returned_fd, send_message);

    ExpectedResultDataValue response_value(reply_data1,
                                           strlen(reply_data1) + 1);

    EXPECT_TRUE(response_value.NextMessagePieceMatches(&reply_message));
    EXPECT_EQ(len, response_value.Size());

    wait_event.Wait();
  }

  // Mixed message 1
  {
    SCOPED_TRACE("Mixed message 1");
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    ExpectedResultDataValue data1_value(data1, strlen(data1) + 1);
    ExpectedResultIntValue int1_value(int1);
    ExpectedResultValue* expected_results[] = {&data1_value, &int1_value};
    message_thread.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&BrokerSimpleMessageTestHelper::RecvMsgAndReply,
                       &ipc_reader, expected_results,
                       arraysize(expected_results), reply_data1, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    BrokerSimpleMessage send_message;
    send_message.AddDataToMessage(data1, strlen(data1) + 1);
    send_message.AddIntToMessage(int1);
    BrokerSimpleMessage reply_message;
    int returned_fd;
    ssize_t len = BrokerSimpleMessage::SendRecvMsgWithFlags(
        ipc_writer.get(), &reply_message, 0, &returned_fd, send_message);

    ExpectedResultDataValue response_value(reply_data1,
                                           strlen(reply_data1) + 1);

    EXPECT_TRUE(response_value.NextMessagePieceMatches(&reply_message));
    EXPECT_EQ(len, response_value.Size());

    wait_event.Wait();
  }

  // Mixed message 2
  {
    SCOPED_TRACE("Mixed message 2");
    BrokerChannel::EndPoint ipc_reader;
    BrokerChannel::EndPoint ipc_writer;
    BrokerChannel::CreatePair(&ipc_reader, &ipc_writer);

    ExpectedResultDataValue data1_value(data1, strlen(data1) + 1);
    ExpectedResultIntValue int1_value(int1);
    ExpectedResultIntValue int2_value(int2);
    ExpectedResultDataValue data2_value(data2, strlen(data2) + 1);
    ExpectedResultValue* expected_results[] = {&data1_value, &int1_value,
                                               &int2_value, &data2_value};
    message_thread.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&BrokerSimpleMessageTestHelper::RecvMsgAndReply,
                       &ipc_reader, expected_results,
                       arraysize(expected_results), reply_data1, -1));

    PostWaitableEventToThread(&message_thread, &wait_event);

    BrokerSimpleMessage send_message;
    send_message.AddDataToMessage(data1, strlen(data1) + 1);
    send_message.AddIntToMessage(int1);
    send_message.AddIntToMessage(int2);
    send_message.AddDataToMessage(data2, strlen(data2) + 1);
    BrokerSimpleMessage reply_message;
    int returned_fd;
    ssize_t len = BrokerSimpleMessage::SendRecvMsgWithFlags(
        ipc_writer.get(), &reply_message, 0, &returned_fd, send_message);

    ExpectedResultDataValue response_value(reply_data1,
                                           strlen(reply_data1) + 1);

    EXPECT_TRUE(response_value.NextMessagePieceMatches(&reply_message));
    EXPECT_EQ(len, response_value.Size());

    wait_event.Wait();
  }
}

}  // namespace syscall_broker

}  // namespace sandbox
