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
#include "storage/browser/fileapi/file_system_context.h"
#include "url/gurl.h"
#include "url/origin.h"

using media::mojom::CdmStorage;

namespace {

const char kExternalClearKey[] = "org.chromium.externalclearkey";

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

  // Open the file |name|. As we're not testing I/O, we close the file
  // immediately in the callback and return whether the file was valid or not.
  bool Open(const std::string& name, CdmStorage::Status* status) {
    DVLOG(3) << __func__;
    bool result;
    cdm_storage_->Open(
        name, base::Bind(&CdmStorageTest::OpenDone, base::Unretained(this),
                         status, &result));
    RunAndWaitForResult();
    return result;
  }

  // Close the file |name|.
  bool Close(const std::string& name) {
    DVLOG(3) << __func__;
    bool result;
    cdm_storage_->Close(name, base::Bind(&CdmStorageTest::CloseDone,
                                         base::Unretained(this), &result));
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

  void CloseDone(bool* result, bool status) {
    DVLOG(3) << __func__;
    *result = status;
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
  EXPECT_FALSE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidFileNameWithBackSlash) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "name\\";
  CdmStorage::Status status;
  EXPECT_FALSE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidFileNameWithUnderscore) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "_name";
  CdmStorage::Status status;
  EXPECT_FALSE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, InvalidFileNameEmpty) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "";
  CdmStorage::Status status;
  EXPECT_FALSE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::FAILURE);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, OpenFile) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  EXPECT_TRUE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, OpenFileLocked) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  EXPECT_TRUE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);

  // Second attempt on the same file should fail as the file is locked.
  EXPECT_FALSE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::IN_USE);

  // Now close the first file and try again. It should be free now.
  EXPECT_TRUE(Close(kFileName));
  EXPECT_TRUE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, CloseFile) {
  DVLOG(3) << __func__;
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  EXPECT_TRUE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(Close(kFileName));
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, CloseFileTwice) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  EXPECT_TRUE(Open(kFileName, &status));
  EXPECT_EQ(status, CdmStorage::Status::SUCCESS);
  EXPECT_TRUE(Close(kFileName));
  EXPECT_FALSE(Close(kFileName));
}

IN_PROC_BROWSER_TEST_F(CdmStorageTest, CloseUnknownFile) {
  EXPECT_TRUE(Initialize(kExternalClearKey));

  const char kFileName[] = "test_file_name";
  EXPECT_FALSE(Close(kFileName));
}
