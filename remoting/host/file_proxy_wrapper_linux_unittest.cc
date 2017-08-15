// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_proxy_wrapper.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "net/base/io_buffer.h"
#include "remoting/base/compound_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kTestFilename[] = "test-file.txt";
constexpr char kTestFilenameSecondary[] = "test-file (1).txt";
const std::string& kTestDataOne = "this is the first test string";
const std::string& kTestDataTwo = "this is the second test string";
const std::string& kTestDataThree = "this is the third test string";

}  // namespace

namespace remoting {

class FileProxyWrapperLinuxTest : public testing::Test {
 public:
  FileProxyWrapperLinuxTest();
  ~FileProxyWrapperLinuxTest() override;

  // testing::Test implementation.
  void SetUp() override;
  void TearDown() override;

  const base::FilePath& TestDir() const { return dir_.GetPath(); }
  const base::FilePath TestFilePath() const {
    return dir_.GetPath().Append(kTestFilename);
  }

  void ErrorCallback(protocol::FileTransferResponse_ErrorCode error);
  void CreateFileCallback();
  void CloseCallback();

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::ScopedTempDir dir_;

  std::unique_ptr<FileProxyWrapper> file_proxy_wrapper_;
  std::unique_ptr<protocol::FileTransferResponse_ErrorCode> error_;
  bool create_callback_called_;
  bool close_callback_called_;
};

FileProxyWrapperLinuxTest::FileProxyWrapperLinuxTest()
    : scoped_task_environment_(
          base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT,
          base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED) {}
FileProxyWrapperLinuxTest::~FileProxyWrapperLinuxTest() = default;

void FileProxyWrapperLinuxTest::SetUp() {
  ASSERT_TRUE(dir_.CreateUniqueTempDir());

  file_proxy_wrapper_ = FileProxyWrapper::Create();
  file_proxy_wrapper_->Init(base::Bind(
      &FileProxyWrapperLinuxTest::ErrorCallback, base::Unretained(this)));

  error_.reset();
  create_callback_called_ = false;
  close_callback_called_ = false;
}

void FileProxyWrapperLinuxTest::TearDown() {
  file_proxy_wrapper_.reset();
}

void FileProxyWrapperLinuxTest::ErrorCallback(
    protocol::FileTransferResponse_ErrorCode error) {
  error_.reset(new protocol::FileTransferResponse_ErrorCode(error));
}

void FileProxyWrapperLinuxTest::CreateFileCallback() {
  create_callback_called_ = true;
}

void FileProxyWrapperLinuxTest::CloseCallback() {
  close_callback_called_ = true;
}

TEST_F(FileProxyWrapperLinuxTest, WriteThreeChunks) {
  file_proxy_wrapper_->CreateFile(
      TestDir(), kTestFilename,
      base::Bind(&FileProxyWrapperLinuxTest::CreateFileCallback,
                 base::Unretained(this)));

  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(error_);
  EXPECT_TRUE(create_callback_called_);

  std::unique_ptr<CompoundBuffer> buffer =
      base::WrapUnique(new CompoundBuffer());
  buffer->Append(new net::WrappedIOBuffer(kTestDataOne.data()),
                 kTestDataOne.size());
  file_proxy_wrapper_->WriteChunk(std::move(buffer));

  buffer = base::WrapUnique(new CompoundBuffer());
  buffer->Append(new net::WrappedIOBuffer(kTestDataTwo.data()),
                 kTestDataTwo.size());
  file_proxy_wrapper_->WriteChunk(std::move(buffer));

  buffer = base::WrapUnique(new CompoundBuffer());
  buffer->Append(new net::WrappedIOBuffer(kTestDataThree.data()),
                 kTestDataThree.size());
  file_proxy_wrapper_->WriteChunk(std::move(buffer));

  file_proxy_wrapper_->Close(base::Bind(
      &FileProxyWrapperLinuxTest::CloseCallback, base::Unretained(this)));

  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(error_);
  EXPECT_TRUE(close_callback_called_);

  std::string actual_file_data;
  EXPECT_TRUE(base::ReadFileToString(TestFilePath(), &actual_file_data));
  EXPECT_TRUE(kTestDataOne + kTestDataTwo + kTestDataThree == actual_file_data);
}

TEST_F(FileProxyWrapperLinuxTest, CancelDeletesFiles) {
  file_proxy_wrapper_->CreateFile(
      TestDir(), kTestFilename,
      base::Bind(&FileProxyWrapperLinuxTest::CreateFileCallback,
                 base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  std::unique_ptr<CompoundBuffer> buffer =
      base::WrapUnique(new CompoundBuffer());
  buffer->Append(new net::WrappedIOBuffer(kTestDataOne.data()),
                 kTestDataOne.size());
  file_proxy_wrapper_->WriteChunk(std::move(buffer));
  scoped_task_environment_.RunUntilIdle();

  file_proxy_wrapper_->Cancel();
  file_proxy_wrapper_.reset();
  scoped_task_environment_.RunUntilIdle();

  EXPECT_TRUE(base::IsDirectoryEmpty(TestDir()));
}

TEST_F(FileProxyWrapperLinuxTest, FileAlreadyExists) {
  WriteFile(TestFilePath(), kTestDataOne.data(), kTestDataOne.size());

  file_proxy_wrapper_->CreateFile(
      TestDir(), kTestFilename,
      base::Bind(&FileProxyWrapperLinuxTest::CreateFileCallback,
                 base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  std::unique_ptr<CompoundBuffer> buffer =
      base::WrapUnique(new CompoundBuffer());
  buffer->Append(new net::WrappedIOBuffer(kTestDataTwo.data()),
                 kTestDataTwo.size());
  file_proxy_wrapper_->WriteChunk(std::move(buffer));
  file_proxy_wrapper_->Close(base::Bind(
      &FileProxyWrapperLinuxTest::CloseCallback, base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  std::string actual_file_data;
  base::FilePath secondary_filepath = TestDir().Append(kTestFilenameSecondary);
  EXPECT_TRUE(base::ReadFileToString(secondary_filepath, &actual_file_data));
  EXPECT_TRUE(kTestDataTwo == actual_file_data);
}

}  // namespace remoting
