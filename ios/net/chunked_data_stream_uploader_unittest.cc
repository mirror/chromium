// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/net/chunked_data_stream_uploader.h"

#include <memory>
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {
const int kDefaultIOBufferSize = 1024;
}

// Mock delegate to provide data from its internal buffer.
class MockChunkedDataStreamUploaderDelegate
    : public ChunkedDataStreamUploader::Delegate {
 public:
  MockChunkedDataStreamUploaderDelegate()
      : data_(new char[kDefaultIOBufferSize]),
        data_length_(0),
        upload_finished_(false) {}
  ~MockChunkedDataStreamUploaderDelegate() override {}

  int OnRead(char* buffer, int buffer_length) override {
    memcpy(buffer, data_, data_length_);
    return data_length_;
  }

  void OnFinishUpload() override { upload_finished_ = true; }

  void SetReadData(const char* data, int data_length) {
    memcpy(data_, data, data_length);
    data_length_ = data_length;
  }

  bool upload_finished() { return upload_finished_; }

 private:
  char* data_;
  int data_length_;

  bool upload_finished_;
};

class ChunkedDataStreamUploaderTest : public testing::Test {
 public:
  ChunkedDataStreamUploaderTest() {}

  void NullCallback(int result) {}

 protected:
  void SetUp() override {
    delegate_.reset(new MockChunkedDataStreamUploaderDelegate);
    uploader_owner_.reset(new ChunkedDataStreamUploader(delegate_.get()));
    uploader_ = uploader_owner_->GetUploader();

    uploader_owner_->Init(
        base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                            base::Unretained(this)),
        net::NetLogWithSource());
  }

  void TearDown() override {}

  std::unique_ptr<MockChunkedDataStreamUploaderDelegate> delegate_;

  std::unique_ptr<ChunkedDataStreamUploader> uploader_owner_;
  base::WeakPtr<ChunkedDataStreamUploader> uploader_;
};

// Tests that data from the application layer become ready before the network
// layer callback.
TEST_F(ChunkedDataStreamUploaderTest, ExternalDataReadyFirst) {
  // Application layer data is ready.
  const char test_data[] = "Hello world!";
  delegate_->SetReadData(test_data, sizeof(test_data));
  uploader_->UploadWhenReady(false);

  // Network layer callback is called next, and the application data is expected
  // to be read to the |buffer|.
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kDefaultIOBufferSize);
  int bytes_read = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(sizeof(test_data), (size_t)bytes_read);
  EXPECT_FALSE(memcmp(test_data, buffer->data(), (size_t)bytes_read));

  // Application finishes data upload. The |OnFinishUpload| is expected to be
  // called after the network layer callback.
  delegate_->SetReadData(test_data, 0);
  uploader_->UploadWhenReady(true);
  bytes_read = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(0, bytes_read);
  EXPECT_TRUE(delegate_->upload_finished());
}

// Tests that callback from the network layer is called before the application
// layer data available.
TEST_F(ChunkedDataStreamUploaderTest, InternalReadReadyFirst) {
  // Network layer callback is called and the request is pending.
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kDefaultIOBufferSize);
  int ret = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(net::ERR_IO_PENDING, ret);

  // The data is writen into |buffer| once the application layer data is ready.
  const char test_data[] = "Hello world!";
  delegate_->SetReadData(test_data, sizeof(test_data));
  uploader_->UploadWhenReady(false);
  EXPECT_FALSE(memcmp(test_data, buffer->data(), sizeof(test_data)));

  // Callback is called again after a successful read.
  ret = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(net::ERR_IO_PENDING, ret);

  // No more data is available, and the upload will be finished.
  delegate_->SetReadData(test_data, 0);
  uploader_->UploadWhenReady(true);
  EXPECT_TRUE(delegate_->upload_finished());
}

// Tests that null data is correctly handled in the callback comes first case.
TEST_F(ChunkedDataStreamUploaderTest, NullContentWithReadFirst) {
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kDefaultIOBufferSize);
  int ret = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(net::ERR_IO_PENDING, ret);

  const char test_data[1] = {0};
  delegate_->SetReadData(test_data, 0);
  uploader_->UploadWhenReady(true);
  EXPECT_TRUE(delegate_->upload_finished());
}

// Tests that null data is correctly handled in the data is available first
// case.
TEST_F(ChunkedDataStreamUploaderTest, NullContentWithDataFirst) {
  const char test_data[1] = {0};
  delegate_->SetReadData(test_data, 0);
  uploader_->UploadWhenReady(true);

  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kDefaultIOBufferSize);
  int bytes_read = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(0, bytes_read);
  EXPECT_TRUE(delegate_->upload_finished());
}

// A complex test case that the application layer data and network layer
// callback becomes ready first reciprocally.
TEST_F(ChunkedDataStreamUploaderTest, MixedScenarioTest) {
  // Data comes first.
  const char test_data[] = "Hello world!";
  delegate_->SetReadData(test_data, sizeof(test_data));
  uploader_->UploadWhenReady(false);

  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kDefaultIOBufferSize);
  int bytes_read = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(sizeof(test_data), (size_t)bytes_read);
  EXPECT_FALSE(memcmp(test_data, buffer->data(), (size_t)bytes_read));

  // Callback comes first.
  int ret = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(net::ERR_IO_PENDING, ret);

  char test_data_long[kDefaultIOBufferSize] = {0};
  for (int i = 0; i < (int)sizeof(test_data_long); ++i) {
    test_data_long[i] = (char)(i & 0xFF);
  }
  delegate_->SetReadData(test_data_long, sizeof(test_data_long));
  uploader_->UploadWhenReady(false);
  for (int i = 0; i < (int)sizeof(test_data_long); ++i) {
    ASSERT_EQ(test_data_long[i], buffer->data()[i]);
  }

  // Callback comes first again.
  ret = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(net::ERR_IO_PENDING, ret);
  delegate_->SetReadData(test_data, sizeof(test_data));
  uploader_->UploadWhenReady(false);
  EXPECT_FALSE(memcmp(test_data, buffer->data(), sizeof(test_data)));

  // Finish and data comes first.
  delegate_->SetReadData(test_data, 0);
  uploader_->UploadWhenReady(true);
  bytes_read = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(0, bytes_read);
  EXPECT_TRUE(delegate_->upload_finished());
}

}  // namespace net
