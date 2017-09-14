// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_impl.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "url/gurl.h"
#include "url/origin.h"

using media::mojom::CdmStorage;

namespace {

const char kExternalClearKey[] = "org.chromium.externalclearkey";

// mojom::CdmStorage takes a mojom::CdmFileInUse reference to keep track of
// the file being used. This class represents a trivial version that simply
// owns the binding.
class FileInUseTracker : public media::mojom::CdmFileInUse {
 public:
  FileInUseTracker() : binding_(this) {}
  ~FileInUseTracker() override = default;

  media::mojom::CdmFileInUsePtr CreateInterfacePtrAndBind() {
    media::mojom::CdmFileInUsePtr file_in_use;
    binding_.Bind(mojo::MakeRequest(&file_in_use));
    return file_in_use;
  }

 private:
  mojo::Binding<media::mojom::CdmFileInUse> binding_;
};

}  // namespace

class CdmStorageTest : public InProcessBrowserTest {
 public:
  CdmStorageTest() {}

  // Initializes the CdmStorage object for the specified |cdm_type|.
  bool Initialize(const std::string& cdm_type) {
    DVLOG(3) << __func__;

    // Not really doing any I/O, but Open() returns a base::File which needs
    // to be closed.
    base::ThreadRestrictions::SetIOAllowed(true);

    // Create the CdmStorageImpl object. |cdm_storage_| will own the resulting
    // object. Unable to use CdmStorageImpl::Create() as the origin returned
    // by GetMainFrame()->GetLastCommittedOrigin() is unique (no page opened).
    content::RenderProcessHost* render_process_host = browser()
                                                          ->tab_strip_model()
                                                          ->GetWebContentsAt(0)
                                                          ->GetMainFrame()
                                                          ->GetProcess();
    new CdmStorageImpl(
        render_process_host, url::Origin(GURL("http://www.test.com")),
        render_process_host->GetStoragePartition()->GetFileSystemContext(),
        mojo::MakeRequest(&cdm_storage_));

    // Now initialize CDM Storage.
    bool result;
    cdm_storage_->Initialize(cdm_type,
                             base::Bind(&CdmStorageTest::InitializeDone,
                                        base::Unretained(this), &result));
    RunAndWaitForResult();
    return result;
  }

  // Open the file |name|, using |tracker| to keep it open. |tracker|
  // should be destroyed to mark the file as closed. As we're not testing I/O,
  // the file descriptor returned is closed immediately in the callback and
  // this method returns whether the file descriptor provided was valid or not.
  bool Open(const std::string& name,
            media::mojom::CdmFileInUsePtr tracker,
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
  void InitializeDone(bool* result, bool status) {
    DVLOG(3) << __func__;
    *result = status;
    run_loop_->Quit();
  }

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

  media::mojom::CdmStoragePtr cdm_storage_;

  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(CdmStorageTest);
};

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidCdmType) {
  EXPECT_FALSE(Initialize("foo"));
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidFileNameWithSlash) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "name/";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidFileNameWithBackSlash) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "name\\";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidFileNameWithUnderscore) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "_name";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidFileNameEmpty) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_FALSE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, OpenFile) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  std::unique_ptr<FileInUseTracker> file_in_use =
      base::MakeUnique<FileInUseTracker>();
  EXPECT_TRUE(
      Open(kFileName, file_in_use->CreateInterfacePtrAndBind(), &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, OpenFileLocked) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

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

IN_PROC_BROWSER_TEST_F(CdmStorageTest, MultipleFiles) {
  EXPECT_TRUE(Initialize(kExternalClearKey));
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
