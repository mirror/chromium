// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/shared_memory_data_consumer_handle.h"

#include <string.h>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "content/public/child/fixed_received_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {
using blink::WebDataConsumerHandle;
using Result = WebDataConsumerHandle::Result;
using Writer = SharedMemoryDataConsumerHandle::Writer;
using BackpressureMode = SharedMemoryDataConsumerHandle::BackpressureMode;
const BackpressureMode kApplyBackpressure =
    SharedMemoryDataConsumerHandle::kApplyBackpressure;
const BackpressureMode kDoNotApplyBackpressure =
    SharedMemoryDataConsumerHandle::kDoNotApplyBackpressure;

const WebDataConsumerHandle::Flags kNone = WebDataConsumerHandle::FlagNone;
const Result kOk = WebDataConsumerHandle::Ok;
const Result kDone = WebDataConsumerHandle::Done;
const Result kShouldWait = WebDataConsumerHandle::ShouldWait;

using ::testing::_;
using ::testing::InSequence;
using ::testing::MockFunction;
using ::testing::Return;
using ::testing::StrictMock;

using Checkpoint = StrictMock<MockFunction<void(int)>>;
using ReceivedData = RequestPeer::ReceivedData;

class Logger final : public base::RefCounted<Logger> {
 public:
  Logger() {}
  void Add(const std::string& entry) { log_ += entry + "\n"; }
  const std::string& log() const { return log_; }

 private:
  friend class base::RefCounted<Logger>;
  ~Logger() {}
  std::string log_;

  DISALLOW_COPY_AND_ASSIGN(Logger);
};

class LoggingFixedReceivedData final : public RequestPeer::ReceivedData {
 public:
  LoggingFixedReceivedData(const char* name,
                           const char* s,
                           scoped_refptr<Logger> logger)
      : name_(name), data_(s, s + strlen(s)), logger_(logger) {}
  ~LoggingFixedReceivedData() override {
    logger_->Add(name_ + " is destructed.");
  }

  const char* payload() const override {
    return data_.empty() ? nullptr : &data_[0];
  }
  int length() const override { return static_cast<int>(data_.size()); }
  int encoded_length() const override { return static_cast<int>(data_.size()); }

 private:
  const std::string name_;
  const std::vector<char> data_;
  scoped_refptr<Logger> logger_;

  DISALLOW_COPY_AND_ASSIGN(LoggingFixedReceivedData);
};

class MockClient : public WebDataConsumerHandle::Client {
 public:
  MOCK_METHOD0(didGetReadable, void());
};

std::string ToString(const void* p, size_t size) {
  const char* q = static_cast<const char*>(p);
  return std::string(q, q + size);
}

class ThreadedSharedMemoryDataConsumerHandleTest : public ::testing::Test {
 protected:
  class ReadDataOperation;
  class ClientImpl final : public WebDataConsumerHandle::Client {
   public:
    explicit ClientImpl(ReadDataOperation* operation) : operation_(operation) {}

    void didGetReadable() override { operation_->ReadData(); }

   private:
    ReadDataOperation* operation_;
  };

  class ReadDataOperation final {
   public:
    typedef WebDataConsumerHandle::Result Result;
    ReadDataOperation(scoped_ptr<WebDataConsumerHandle> handle,
                      base::MessageLoop* main_message_loop,
                      const base::Closure& on_done)
        : handle_(handle.Pass()),
          main_message_loop_(main_message_loop),
          on_done_(on_done) {}

    const std::string& result() const { return result_; }

    void ReadData() {
      if (!client_) {
        client_.reset(new ClientImpl(this));
        handle_->registerClient(client_.get());
      }

      Result rv = kOk;
      size_t read_size = 0;

      while (true) {
        char buffer[16];
        rv = handle_->read(&buffer, sizeof(buffer), kNone, &read_size);
        if (rv != kOk)
          break;
        result_.insert(result_.size(), &buffer[0], read_size);
      }

      if (rv == kShouldWait) {
        // Wait a while...
        return;
      }

      if (rv != kDone) {
        // Something is wrong.
        result_ = "error";
      }

      // The operation is done.
      main_message_loop_->PostTask(FROM_HERE, on_done_);
    }

   private:
    scoped_ptr<WebDataConsumerHandle> handle_;
    scoped_ptr<WebDataConsumerHandle::Client> client_;
    base::MessageLoop* main_message_loop_;
    base::Closure on_done_;
    std::string result_;
  };

  void SetUp() override {
    handle_.reset(
        new SharedMemoryDataConsumerHandle(kApplyBackpressure, &writer_));
  }

  StrictMock<MockClient> client_;
  scoped_ptr<WebDataConsumerHandle> handle_;
  scoped_ptr<Writer> writer_;
  base::MessageLoop loop_;
};

class SharedMemoryDataConsumerHandleTest
    : public ::testing::TestWithParam<BackpressureMode> {
 protected:
  void SetUp() override {
    handle_.reset(new SharedMemoryDataConsumerHandle(GetParam(), &writer_));
  }
  scoped_ptr<FixedReceivedData> NewFixedData(const char* s) {
    return make_scoped_ptr(new FixedReceivedData(s, strlen(s), strlen(s)));
  }

  StrictMock<MockClient> client_;
  scoped_ptr<SharedMemoryDataConsumerHandle> handle_;
  scoped_ptr<Writer> writer_;
  base::MessageLoop loop_;
};

TEST_P(SharedMemoryDataConsumerHandleTest, ReadFromEmpty) {
  char buffer[4];
  size_t read = 88;
  Result result = handle_->read(buffer, 4, kNone, &read);

  EXPECT_EQ(kShouldWait, result);
  EXPECT_EQ(0u, read);
}

TEST_P(SharedMemoryDataConsumerHandleTest, AutoClose) {
  char buffer[4];
  size_t read = 88;

  writer_.reset();
  Result result = handle_->read(buffer, 4, kNone, &read);

  EXPECT_EQ(kDone, result);
  EXPECT_EQ(0u, read);
}

TEST_P(SharedMemoryDataConsumerHandleTest, ReadSimple) {
  writer_->AddData(NewFixedData("hello"));

  char buffer[4] = {};
  size_t read = 88;
  Result result = handle_->read(buffer, 3, kNone, &read);

  EXPECT_EQ(kOk, result);
  EXPECT_EQ(3u, read);
  EXPECT_STREQ("hel", buffer);

  result = handle_->read(buffer, 3, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(2u, read);
  EXPECT_STREQ("lol", buffer);

  result = handle_->read(buffer, 3, kNone, &read);
  EXPECT_EQ(kShouldWait, result);
  EXPECT_EQ(0u, read);

  writer_->Close();

  result = handle_->read(buffer, 3, kNone, &read);
  EXPECT_EQ(kDone, result);
  EXPECT_EQ(0u, read);
}

TEST_P(SharedMemoryDataConsumerHandleTest, CloseBeforeReading) {
  writer_->AddData(NewFixedData("hello"));
  writer_->Close();

  char buffer[20] = {};
  size_t read = 88;
  Result result = handle_->read(buffer, sizeof(buffer), kNone, &read);

  EXPECT_EQ(kOk, result);
  EXPECT_EQ(5u, read);
  EXPECT_STREQ("hello", buffer);

  result = handle_->read(buffer, sizeof(buffer), kNone, &read);
  EXPECT_EQ(kDone, result);
  EXPECT_EQ(0u, read);
}

TEST_P(SharedMemoryDataConsumerHandleTest, AddMultipleData) {
  writer_->AddData(NewFixedData("Once "));
  writer_->AddData(NewFixedData("upon "));
  writer_->AddData(NewFixedData("a "));
  writer_->AddData(NewFixedData("time "));
  writer_->AddData(NewFixedData("there "));
  writer_->AddData(NewFixedData("was "));
  writer_->AddData(NewFixedData("a "));
  writer_->Close();

  char buffer[20];
  size_t read;
  Result result;

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 6, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(6u, read);
  EXPECT_STREQ("Once u", buffer);

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 2, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(2u, read);
  EXPECT_STREQ("po", buffer);

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 9, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(9u, read);
  EXPECT_STREQ("n a time ", buffer);

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 3, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(3u, read);
  EXPECT_STREQ("the", buffer);

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 20, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(9u, read);
  EXPECT_STREQ("re was a ", buffer);

  result = handle_->read(buffer, sizeof(buffer), kNone, &read);
  EXPECT_EQ(kDone, result);
  EXPECT_EQ(0u, read);
}

TEST_P(SharedMemoryDataConsumerHandleTest, AddMultipleDataInteractively) {
  writer_->AddData(NewFixedData("Once "));
  writer_->AddData(NewFixedData("upon "));

  char buffer[20];
  size_t read;
  Result result;

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 6, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(6u, read);
  EXPECT_STREQ("Once u", buffer);

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 2, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(2u, read);
  EXPECT_STREQ("po", buffer);

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 9, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(2u, read);
  EXPECT_STREQ("n ", buffer);

  writer_->AddData(NewFixedData("a "));

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 1, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(1u, read);
  EXPECT_STREQ("a", buffer);

  writer_->AddData(NewFixedData("time "));
  writer_->AddData(NewFixedData("there "));
  writer_->AddData(NewFixedData("was "));
  writer_->AddData(NewFixedData("a "));
  writer_->Close();

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 9, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(9u, read);
  EXPECT_STREQ(" time the", buffer);

  std::fill(&buffer[0], &buffer[arraysize(buffer)], 0);
  result = handle_->read(buffer, 20, kNone, &read);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(9u, read);
  EXPECT_STREQ("re was a ", buffer);

  result = handle_->read(buffer, sizeof(buffer), kNone, &read);
  EXPECT_EQ(kDone, result);
  EXPECT_EQ(0u, read);
}

TEST_P(SharedMemoryDataConsumerHandleTest, RegisterClient) {
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(0));
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(client_, didGetReadable());
  EXPECT_CALL(checkpoint, Call(2));

  checkpoint.Call(0);
  handle_->registerClient(&client_);
  checkpoint.Call(1);
  writer_->Close();
  checkpoint.Call(2);
}

TEST_P(SharedMemoryDataConsumerHandleTest, RegisterClientWhenDataExists) {
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(0));
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(client_, didGetReadable());
  EXPECT_CALL(checkpoint, Call(2));

  checkpoint.Call(0);
  writer_->AddData(NewFixedData("Once "));
  checkpoint.Call(1);
  handle_->registerClient(&client_);
  checkpoint.Call(2);
}

TEST_P(SharedMemoryDataConsumerHandleTest, AddDataWhenClientIsRegistered) {
  Checkpoint checkpoint;
  char buffer[20];
  Result result;
  size_t size;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(0));
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(client_, didGetReadable());
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(checkpoint, Call(4));
  EXPECT_CALL(client_, didGetReadable());
  EXPECT_CALL(checkpoint, Call(5));

  checkpoint.Call(0);
  handle_->registerClient(&client_);
  checkpoint.Call(1);
  writer_->AddData(NewFixedData("Once "));
  checkpoint.Call(2);
  writer_->AddData(NewFixedData("upon "));
  checkpoint.Call(3);
  result = handle_->read(buffer, sizeof(buffer), kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(10u, size);
  checkpoint.Call(4);
  writer_->AddData(NewFixedData("a "));
  checkpoint.Call(5);
}

TEST_P(SharedMemoryDataConsumerHandleTest, CloseWithClientAndData) {
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(0));
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(client_, didGetReadable());
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(0);
  handle_->registerClient(&client_);
  checkpoint.Call(1);
  writer_->AddData(NewFixedData("Once "));
  checkpoint.Call(2);
  writer_->Close();
  checkpoint.Call(3);
}

TEST_P(SharedMemoryDataConsumerHandleTest, UnregisterClient) {
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(0));
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(checkpoint, Call(2));

  checkpoint.Call(0);
  handle_->registerClient(&client_);
  checkpoint.Call(1);
  handle_->unregisterClient();
  writer_->AddData(NewFixedData("Once "));
  checkpoint.Call(2);
}

TEST_P(SharedMemoryDataConsumerHandleTest, TwoPhaseReadShouldWait) {
  Result result;
  const void* buffer = &result;
  size_t size = 99;

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kShouldWait, result);
  EXPECT_EQ(nullptr, buffer);
  EXPECT_EQ(0u, size);
}

TEST_P(SharedMemoryDataConsumerHandleTest, TwoPhaseReadSimple) {
  writer_->AddData(NewFixedData("Once "));

  Result result;
  const void* buffer = &result;
  size_t size = 99;

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(5u, size);
  EXPECT_EQ("Once ", ToString(buffer, 5));

  handle_->endRead(1);

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(4u, size);
  EXPECT_EQ("nce ", ToString(buffer, 4));

  handle_->endRead(4);

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kShouldWait, result);
  EXPECT_EQ(0u, size);
  EXPECT_EQ(nullptr, buffer);

  writer_->Close();

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kDone, result);
  EXPECT_EQ(0u, size);
  EXPECT_EQ(nullptr, buffer);
}

TEST_P(SharedMemoryDataConsumerHandleTest, TwoPhaseReadWithMultipleData) {
  writer_->AddData(NewFixedData("Once "));
  writer_->AddData(NewFixedData("upon "));

  Result result;
  const void* buffer = &result;
  size_t size = 99;

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(5u, size);
  EXPECT_EQ("Once ", ToString(buffer, 5));

  handle_->endRead(1);

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(4u, size);
  EXPECT_EQ("nce ", ToString(buffer, 4));

  handle_->endRead(4);

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(5u, size);
  EXPECT_EQ("upon ", ToString(buffer, 5));

  handle_->endRead(5);

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kShouldWait, result);
  EXPECT_EQ(0u, size);
  EXPECT_EQ(nullptr, buffer);

  writer_->Close();

  result = handle_->beginRead(&buffer, kNone, &size);
  EXPECT_EQ(kDone, result);
  EXPECT_EQ(0u, size);
  EXPECT_EQ(nullptr, buffer);
}

TEST(SharedMemoryDataConsumerHandleBackpressureTest, Read) {
  base::MessageLoop loop;
  char buffer[20];
  Result result;
  size_t size;

  scoped_ptr<Writer> writer;
  auto handle = make_scoped_ptr(
      new SharedMemoryDataConsumerHandle(kApplyBackpressure, &writer));
  scoped_refptr<Logger> logger(new Logger);
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data1", "Once ", logger)));
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data2", "upon ", logger)));
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data3", "a ", logger)));
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data4", "time ", logger)));

  logger->Add("1");
  result = handle->read(buffer, 2, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(2u, size);
  logger->Add("2");
  result = handle->read(buffer, 5, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(5u, size);
  logger->Add("3");
  result = handle->read(buffer, 6, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(6u, size);
  logger->Add("4");

  EXPECT_EQ(
      "1\n"
      "2\n"
      "data1 is destructed.\n"
      "3\n"
      "data2 is destructed.\n"
      "data3 is destructed.\n"
      "4\n",
      logger->log());
}

TEST(SharedMemoryDataConsumerHandleBackpressureTest, CloseAndReset) {
  base::MessageLoop loop;
  char buffer[20];
  Result result;
  size_t size;

  scoped_ptr<Writer> writer;
  auto handle = make_scoped_ptr(
      new SharedMemoryDataConsumerHandle(kApplyBackpressure, &writer));
  scoped_refptr<Logger> logger(new Logger);
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data1", "Once ", logger)));
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data2", "upon ", logger)));
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data3", "a ", logger)));

  logger->Add("1");
  result = handle->read(buffer, 2, kNone, &size);
  EXPECT_EQ(kOk, result);
  EXPECT_EQ(2u, size);
  logger->Add("2");
  writer->Close();
  logger->Add("3");
  handle.reset();
  logger->Add("4");

  EXPECT_EQ(
      "1\n"
      "2\n"
      "3\n"
      "data1 is destructed.\n"
      "data2 is destructed.\n"
      "data3 is destructed.\n"
      "4\n",
      logger->log());
}

TEST(SharedMemoryDataConsumerHandleWithoutBackpressureTest, AddData) {
  base::MessageLoop loop;
  scoped_ptr<Writer> writer;
  auto handle = make_scoped_ptr(
      new SharedMemoryDataConsumerHandle(kDoNotApplyBackpressure, &writer));
  scoped_refptr<Logger> logger(new Logger);

  logger->Add("1");
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data1", "Once ", logger)));
  logger->Add("2");
  writer->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data2", "upon ", logger)));
  logger->Add("3");

  EXPECT_EQ(
      "1\n"
      "data1 is destructed.\n"
      "2\n"
      "data2 is destructed.\n"
      "3\n",
      logger->log());
}

TEST_F(ThreadedSharedMemoryDataConsumerHandleTest, Read) {
  base::RunLoop run_loop;
  auto operation = make_scoped_ptr(
      new ReadDataOperation(handle_.Pass(), &loop_, run_loop.QuitClosure()));
  scoped_refptr<Logger> logger(new Logger);

  base::Thread t("DataConsumerHandle test thread");
  ASSERT_TRUE(t.Start());

  t.message_loop()->PostTask(FROM_HERE,
                             base::Bind(&ReadDataOperation::ReadData,
                                        base::Unretained(operation.get())));

  logger->Add("1");
  writer_->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data1", "Once ", logger)));
  writer_->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data2", "upon ", logger)));
  writer_->AddData(make_scoped_ptr(
      new LoggingFixedReceivedData("data3", "a time ", logger)));
  writer_->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data4", "there ", logger)));
  writer_->AddData(
      make_scoped_ptr(new LoggingFixedReceivedData("data5", "was ", logger)));
  writer_->Close();
  logger->Add("2");

  run_loop.Run();
  t.Stop();

  EXPECT_EQ("Once upon a time there was ", operation->result());
  EXPECT_EQ(
      "1\n"
      "2\n"
      "data1 is destructed.\n"
      "data2 is destructed.\n"
      "data3 is destructed.\n"
      "data4 is destructed.\n"
      "data5 is destructed.\n",
      logger->log());
}

INSTANTIATE_TEST_CASE_P(SharedMemoryDataConsumerHandleTest,
                        SharedMemoryDataConsumerHandleTest,
                        ::testing::Values(kApplyBackpressure,
                                          kDoNotApplyBackpressure));
}  // namespace

}  // namespace content
