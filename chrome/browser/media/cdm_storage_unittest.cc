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

// mojom::CdmStorage takes a mojom::CdmFileInUse reference to keep track of
// the file being used. This class represents a trivial version that simply
// owns the binding.
class FileInUseTracker : public media::mojom::CdmFileReleaser {
 public:
  FileInUseTracker() : binding_(this) {}
  ~FileInUseTracker() override = default;

  media::mojom::CdmFileReleaserPtr CreateInterfacePtrAndBind() {
    media::mojom::CdmFileReleaserPtr file_in_use;
    binding_.Bind(mojo::MakeRequest(&file_in_use));
    return file_in_use;
  }

 private:
  mojo::Binding<media::mojom::CdmFileReleaser> binding_;
};

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
            media::mojom::CdmFileReleaserPtr tracker,
            CdmStorage::Status* status) {
    DVLOG(3) << __func__;
    bool result;

    cdm_storage_->Open(name, std::move(tracker),
                       base::Bind(&CdmStorageTest::OpenDone,
                                  base::Unretained(this), status, &result));
    RunAndWaitForResult();
    return result;
  }

 private:
  void OpenDone(CdmStorage::Status* status,
                bool* result,
                CdmStorage::Status actual_status,
                base::File actual_file) {
    DVLOG(3) << __func__;
    *status = actual_status;
    *result = actual_file.IsValid();
    actual_file.Close();
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
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

TEST_F(CdmStorageTest, InvalidFileNameWithSlash) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "name/";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

TEST_F(CdmStorageTest, InvalidFileNameWithBackSlash) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "name\\";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

TEST_F(CdmStorageTest, InvalidFileNameWithUnderscore) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "_name";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

TEST_F(CdmStorageTest, InvalidFileNameEmpty) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

TEST_F(CdmStorageTest, OpenFile) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_TRUE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
}

TEST_F(CdmStorageTest, OpenFileLocked) {
  Initialize(kExternalClearKey);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file1_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_TRUE(
      Open(kFileName, file1_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);

  // Second attempt on the same file should fail as the file is locked.
  std::unique_ptr<FileInUseTracker> file2_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file2_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::IN_USE);

  // Now close the first file and try again. It should be free now.
  file1_in_use.reset();
  std::unique_ptr<FileInUseTracker> file3_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_TRUE(
      Open(kFileName, file3_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
}

TEST_F(CdmStorageTest, MultipleFiles) {
  Initialize(kExternalClearKey);
  CdmStorage::Status status;

  const char kFileName1[] = "file1";
  std::unique_ptr<FileInUseTracker> file1_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_TRUE(
      Open(kFileName1, file1_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);

  const char kFileName2[] = "file2";
  std::unique_ptr<FileInUseTracker> file2_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_TRUE(
      Open(kFileName2, file2_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);

  const char kFileName3[] = "file3";
  std::unique_ptr<FileInUseTracker> file3_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_TRUE(
      Open(kFileName3, file3_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
}
