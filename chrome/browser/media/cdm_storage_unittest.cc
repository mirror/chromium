// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_impl.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using media::mojom::CdmStorage;

namespace {

const char kExternalClearKey[] = "org.chromium.externalclearkey";

// As these tests aren't doing actual FileIO, we don't care what the renderer
// process ID is, so pick a default value.
const int kRendererProcessId = 0;

}  // namespace

class CdmStorageTest : public testing::Test {
 public:
  CdmStorageTest() {
    // Create a filesystem to test with.
    profile_.reset(new TestingProfile());
    filesystem_context_ =
        content::BrowserContext::GetDefaultStoragePartition(profile_.get())
            ->GetFileSystemContext();
    base::RunLoop().RunUntilIdle();
  }

  ~CdmStorageTest() override {
    // Avoid memory leaks.
    profile_.reset();
    base::RunLoop().RunUntilIdle();
  }

  // Creates and initializes the CdmStorage object for the specified
  // |key_system|.
  void Initialize(const std::string& key_system) {
    DVLOG(3) << __func__;

    // Not really doing any I/O, but Open() returns a base::File which needs
    // to be closed.
    base::ThreadRestrictions::SetIOAllowed(true);

    // Create the CdmStorageImpl object. |cdm_storage_| will own the resulting
    // object. Unable to use CdmStorageImpl::Create() as there is no
    // content::RenderProcessHost to use.
    new CdmStorageImpl(kRendererProcessId,
                       url::Origin(GURL("http://www.test.com")),
                       filesystem_context_, mojo::MakeRequest(&cdm_storage_));

    // Now initialize CDM Storage.
    cdm_storage_->Initialize(key_system);
  }

  // Open the file |name|, using |tracker| to keep it open. |tracker|
  // should be destroyed to mark the file as closed. As we're not testing I/O,
  // the file descriptor returned is closed immediately in the callback and
  // this method returns whether the file descriptor provided was valid or not.
  bool Open(const std::string& name,
            CdmStorage::Status* status,
            base::File* file,
            media::mojom::CdmFileReleaserPtr* tracker) {
    DVLOG(3) << __func__;

    cdm_storage_->Open(
        name, base::Bind(&CdmStorageTest::OpenDone, base::Unretained(this),
                         status, file, tracker));
    RunAndWaitForResult();
    return file->IsValid();
  }

 private:
  void OpenDone(CdmStorage::Status* status,
                base::File* file,
                media::mojom::CdmFileReleaserPtr* tracker,
                CdmStorage::Status actual_status,
                base::File actual_file,
                media::mojom::CdmFileReleaserPtr actual_tracker) {
    DVLOG(3) << __func__;
    *status = actual_status;
    *file = std::move(actual_file);
    *tracker = std::move(actual_tracker);
    run_loop_->Quit();
  }

  // Start running and allow the asynchronous IO operations to complete.
  void RunAndWaitForResult() {
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  storage::FileSystemContext* filesystem_context_;
  media::mojom::CdmStoragePtr cdm_storage_;

  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(CdmStorageTest);
};

TEST_F(CdmStorageTest, InvalidCdmType) {
  Initialize("foo");

  const char kFileName[] = "name";
  CdmStorage::Status status;
  base::File file;
  media::mojom::CdmFileReleaserPtr tracker;
  EXPECT_FALSE(Open(kFileName, &status, &file, &tracker));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(tracker.is_bound());
}

TEST_F(CdmStorageTest, InvalidFileNameWithSlash) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "name/";
  CdmStorage::Status status;
  base::File file;
  media::mojom::CdmFileReleaserPtr tracker;
  EXPECT_FALSE(Open(kFileName, &status, &file, &tracker));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(tracker.is_bound());
}

TEST_F(CdmStorageTest, InvalidFileNameWithBackSlash) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "name\\";
  CdmStorage::Status status;
  base::File file;
  media::mojom::CdmFileReleaserPtr tracker;
  EXPECT_FALSE(Open(kFileName, &status, &file, &tracker));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(tracker.is_bound());
}

TEST_F(CdmStorageTest, InvalidFileNameWithUnderscore) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "_name";
  CdmStorage::Status status;
  base::File file;
  media::mojom::CdmFileReleaserPtr tracker;
  EXPECT_FALSE(Open(kFileName, &status, &file, &tracker));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(tracker.is_bound());
}

TEST_F(CdmStorageTest, InvalidFileNameEmpty) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "";
  CdmStorage::Status status;
  base::File file;
  media::mojom::CdmFileReleaserPtr tracker;
  EXPECT_FALSE(Open(kFileName, &status, &file, &tracker));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(tracker.is_bound());
}

TEST_F(CdmStorageTest, OpenFile) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  base::File file;
  media::mojom::CdmFileReleaserPtr tracker;
  EXPECT_TRUE(Open(kFileName, &status, &file, &tracker));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(file.IsValid());
  EXPECT_TRUE(tracker.is_bound());
}

TEST_F(CdmStorageTest, OpenFileLocked) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  base::File file1;
  media::mojom::CdmFileReleaserPtr tracker1;
  EXPECT_TRUE(Open(kFileName, &status, &file1, &tracker1));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(file1.IsValid());
  EXPECT_TRUE(tracker1.is_bound());

  // Second attempt on the same file should fail as the file is locked.
  base::File file2;
  media::mojom::CdmFileReleaserPtr tracker2;
  EXPECT_FALSE(Open(kFileName, &status, &file2, &tracker2));
  EXPECT_EQ(status, CdmStorage::Status::IN_USE);
  EXPECT_FALSE(file2.IsValid());
  EXPECT_FALSE(tracker2.is_bound());

  // Now close the first file and try again. It should be free now.
  file1.Close();
  tracker1.reset();

  base::File file3;
  media::mojom::CdmFileReleaserPtr tracker3;
  EXPECT_TRUE(Open(kFileName, &status, &file3, &tracker3));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(file3.IsValid());
  EXPECT_TRUE(tracker3.is_bound());
}

TEST_F(CdmStorageTest, MultipleFiles) {
  Initialize(kExternalClearKey);

  const char kFileName1[] = "file1";
  CdmStorage::Status status;
  base::File file1;
  media::mojom::CdmFileReleaserPtr tracker1;
  EXPECT_TRUE(Open(kFileName1, &status, &file1, &tracker1));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(file1.IsValid());
  EXPECT_TRUE(tracker1.is_bound());

  const char kFileName2[] = "file2";
  base::File file2;
  media::mojom::CdmFileReleaserPtr tracker2;
  EXPECT_TRUE(Open(kFileName2, &status, &file2, &tracker2));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(file2.IsValid());
  EXPECT_TRUE(tracker2.is_bound());

  const char kFileName3[] = "file3";
  base::File file3;
  media::mojom::CdmFileReleaserPtr tracker3;
  EXPECT_TRUE(Open(kFileName3, &status, &file3, &tracker3));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(file3.IsValid());
  EXPECT_TRUE(tracker3.is_bound());
}

TEST_F(CdmStorageTest, ReadWriteFile) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  base::File file;
  media::mojom::CdmFileReleaserPtr tracker;
  EXPECT_TRUE(Open(kFileName, &status, &file, &tracker));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(file.IsValid());
  EXPECT_TRUE(tracker.is_bound());

  // Write several bytes and read them back.
  const char kTestData[] = "random string";
  const int kTestDataSize = sizeof(kTestData);
  EXPECT_EQ(kTestDataSize, file.Write(0, kTestData, kTestDataSize));

  char data_read[32];
  const int kTestDataReadSize = sizeof(data_read);
  EXPECT_GT(kTestDataReadSize, kTestDataSize);
  EXPECT_EQ(kTestDataSize, file.Read(0, data_read, kTestDataReadSize));
  for (size_t i = 0; i < kTestDataSize; i++)
    EXPECT_EQ(kTestData[i], data_read[i]);
}
