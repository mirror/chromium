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

class MockChunkedDataStreamUploaderDelegate
    : public ChunkedDataStreamUploader::Delegate {
 public:
  MockChunkedDataStreamUploaderDelegate()
      : data_(new char[kDefaultIOBufferSize]),
        data_len_(0),
        upload_finished_(false) {}
  ~MockChunkedDataStreamUploaderDelegate() override {}

  int OnRead(char* buf, int buf_len) override {
    memcpy(buf, data_, data_len_);
    return data_len_;
  }

  void OnFinishUpload() override { upload_finished_ = true; }

  void SetReadData(const char* data, int data_len) {
    memcpy(data_, data, data_len);
    data_len_ = data_len;
  }

  bool upload_finished() { return upload_finished_; }

 private:
  char* data_;
  int data_len_;

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

TEST_F(ChunkedDataStreamUploaderTest, ExternalDataReadyFirst) {
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

  delegate_->SetReadData(test_data, 0);
  uploader_->UploadWhenReady(true);
  bytes_read = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(0, bytes_read);
  EXPECT_TRUE(delegate_->upload_finished());
}

TEST_F(ChunkedDataStreamUploaderTest, InternalReadReadyFirst) {
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kDefaultIOBufferSize);
  int ret = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(net::ERR_IO_PENDING, ret);

  const char test_data[] = "Hello world!";
  delegate_->SetReadData(test_data, sizeof(test_data));
  uploader_->UploadWhenReady(false);
  EXPECT_FALSE(memcmp(test_data, buffer->data(), sizeof(test_data)));

  ret = uploader_->Read(
      buffer.get(), kDefaultIOBufferSize,
      base::BindRepeating(&ChunkedDataStreamUploaderTest::NullCallback,
                          base::Unretained(this)));
  EXPECT_EQ(net::ERR_IO_PENDING, ret);

  delegate_->SetReadData(test_data, 0);
  uploader_->UploadWhenReady(true);
  EXPECT_TRUE(delegate_->upload_finished());
}

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
