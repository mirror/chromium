// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_file_io.h"

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "media/cdm/api/content_decryption_module.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::SaveArg;
using ::testing::StrictMock;
using Status = cdm::FileIOClient::Status;

namespace media {

namespace {

// Check that the data returned by OnReadComplete() matches |data|.
ACTION_P(CheckData, data) {
  // Args are OnReadComplete(Status, const uint8_t*, uint32_t).
  EXPECT_EQ(arg2, data.size());
  EXPECT_EQ(data, std::vector<uint8_t>(arg1, arg1 + arg2));
}

class MockFileIOClient : public cdm::FileIOClient {
 public:
  MockFileIOClient() {}
  ~MockFileIOClient() override {}

  MOCK_METHOD1(OnOpenComplete, void(Status));
  MOCK_METHOD3(OnReadComplete, void(Status, const uint8_t*, uint32_t));
  MOCK_METHOD1(OnWriteComplete, void(Status));
};

class MockCdmStorage : public mojom::CdmStorage::CdmStorage {
 public:
  MockCdmStorage() {}
  ~MockCdmStorage() override {}

  bool SetUp() { return temp_directory_.CreateUniqueTempDir(); }

  // MojoCdmFileIO calls CdmStorage::Open() to actually open the file. Simulate
  // this by creating a temporary file and returning it. Note that |file_name|
  // is ignored, but it doesn't matter as MojoCdmFileIO only works with a
  // single file anyway.
  void Open(const std::string& /* file_name */,
            OpenCallback callback) override {
    base::FilePath temp_file;
    ASSERT_TRUE(
        base::CreateTemporaryFileInDir(temp_directory_.GetPath(), &temp_file));
    DVLOG(1) << __func__ << " " << temp_file;
    std::move(callback).Run(
        mojom::CdmStorage::Status::kSuccess,
        base::File(temp_file, base::File::FLAG_CREATE_ALWAYS |
                                  base::File::FLAG_READ |
                                  base::File::FLAG_WRITE),
        nullptr);
  }

 private:
  base::ScopedTempDir temp_directory_;
};

}  // namespace

class MojoCdmFileIOTest : public testing::Test {
 protected:
  MojoCdmFileIOTest() {}
  ~MojoCdmFileIOTest() override {}

  void SetUp() override {
    client_ = std::make_unique<MockFileIOClient>();
    cdm_storage_ = std::make_unique<MockCdmStorage>();
    ASSERT_TRUE(cdm_storage_->SetUp());
    file_io_ =
        std::make_unique<MojoCdmFileIO>(client_.get(), cdm_storage_.get());
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<MojoCdmFileIO> file_io_;
  std::unique_ptr<MockCdmStorage> cdm_storage_;
  std::unique_ptr<MockFileIOClient> client_;
};

TEST_F(MojoCdmFileIOTest, OpenFile) {
  const std::string kFileName = "openfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenFileTwice) {
  const std::string kFileName = "openfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());

  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenFileAfterOpen) {
  const std::string kFileName = "openfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());

  // Run now so that the file is opened.
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());

  // Run a second time so Open() tries after the file is already open.
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenDifferentFiles) {
  const std::string kFileName1 = "openfile1";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName1.data(), kFileName1.length());

  const std::string kFileName2 = "openfile2";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName2.data(), kFileName2.length());

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenBadFileName) {
  // Anything other than ASCII letter, digits, and -._ will fail. Add a
  // Unicode character to the name.
  const std::string kFileName = "openfile\u1234";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenTooLongFileName) {
  // Limit is 256 characters, so try a file name with 257.
  const std::string kFileName(257, 'a');
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, Read) {
  const std::string kFileName = "readfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();

  // File doesn't exist, so reading it should return 0 length buffer.
  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kSuccess, _, 0));
  file_io_->Read();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, ReadBeforeOpen) {
  // File not open, so reading should fail.
  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kError, _, _));
  file_io_->Read();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, SimultaneousReads) {
  const std::string kFileName = "readfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kSuccess, _, 0));
  file_io_->Read();
  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kInUse, _, _));
  file_io_->Read();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, Write) {
  const std::string kFileName = "writefile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();

  const std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_CALL(*client_.get(), OnWriteComplete(Status::kSuccess));
  file_io_->Write(data.data(), data.size());
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, WriteBeforeOpen) {
  // File not open, so writing should fail.
  const std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_CALL(*client_.get(), OnWriteComplete(Status::kError));
  file_io_->Write(data.data(), data.size());
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, SimultaneousWrites) {
  const std::string kFileName = "writefile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();

  const std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_CALL(*client_.get(), OnWriteComplete(Status::kSuccess));
  file_io_->Write(data.data(), data.size());
  EXPECT_CALL(*client_.get(), OnWriteComplete(Status::kInUse));
  file_io_->Write(data.data(), data.size());
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, WriteThenRead) {
  const std::string kFileName = "testfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();

  // Write some data to the file.
  const std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 255, 0, 'z'};
  EXPECT_CALL(*client_.get(), OnWriteComplete(Status::kSuccess));
  file_io_->Write(data.data(), data.size());
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kSuccess, _, _))
      .WillOnce(CheckData(data));
  file_io_->Read();
  base::RunLoop().RunUntilIdle();
}

}  // namespace media
