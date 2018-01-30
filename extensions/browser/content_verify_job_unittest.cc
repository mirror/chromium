// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/version.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/content_hash_reader.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_paths.h"
#include "extensions/common/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/google/zip.h"

namespace extensions {

namespace {

// Specifies how test ContentVerifyJob's asynchronous steps to read hash and
// read contents are ordered.
// Note that:
// OnHashesReady: is called when hash reading is complete.
// BytesRead + DoneReading: are called when content reading is complete.
enum ContentVerifyJobAsyncRunMode {
  // None - Let hash reading and content reading continue as is asynchronously.
  kNone,
  // Hashes become available after the contents become available.
  kContentReadBeforeHashesReady,
  // The contents become available before the hashes are ready.
  kHashesReadyBeforeContentRead,
};

scoped_refptr<ContentHashReader> CreateContentHashReader(
    const Extension& extension,
    const base::FilePath& extension_resource_path) {
  return base::MakeRefCounted<ContentHashReader>(
      extension.id(), extension.version(), extension.path(),
      extension_resource_path,
      ContentVerifierKey(kWebstoreSignaturesPublicKey,
                         kWebstoreSignaturesPublicKeySize));
}

void DoNothingWithReasonParam(ContentVerifyJob::FailureReason reason) {}

// A test ContentVerifyJob that provides option to wait for hashes to become
// ready (through WaitForOnHashesReady).
class TestContentVerifyJob : public ContentVerifyJob {
 public:
  TestContentVerifyJob(bool needs_to_observe_hashes_ready,
                       ContentHashReader* hash_reader,
                       FailureCallback failure_callback)
      : ContentVerifyJob(hash_reader, std::move(failure_callback)),
        needs_to_observe_hashes_ready_(needs_to_observe_hashes_ready) {}

  void OnHashesReady(bool success) override {
    ContentVerifyJob::OnHashesReady(success);
    if (needs_to_observe_hashes_ready_)
      run_loop_.Quit();
  }

  void WaitForOnHashesReady() {
    DCHECK(needs_to_observe_hashes_ready_);
    // Run() returns immediately if Quit() has already been called.
    run_loop_.Run();
  }

 protected:
  ~TestContentVerifyJob() override {}

 private:
  // Used when |needs_to_observe_hashes_ready| is true.
  base::RunLoop run_loop_;

  const bool needs_to_observe_hashes_ready_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestContentVerifyJob);
};

class JobTestObserver : public ContentVerifyJob::TestObserver {
 public:
  JobTestObserver(const std::string& extension_id,
                  const base::FilePath& relative_path)
      : extension_id_(extension_id), relative_path_(relative_path) {
    ContentVerifyJob::SetObserverForTests(this);
  }
  ~JobTestObserver() { ContentVerifyJob::SetObserverForTests(nullptr); }

  void JobStarted(const std::string& extension_id,
                  const base::FilePath& relative_path) override {}

  void JobFinished(const std::string& extension_id,
                   const base::FilePath& relative_path,
                   ContentVerifyJob::FailureReason reason) override {
    if (extension_id != extension_id_ || relative_path != relative_path_)
      return;
    failure_reason_ = reason;
    run_loop_.Quit();
  }

  ContentVerifyJob::FailureReason WaitAndGetFailureReason() {
    // Run() returns immediately if Quit() has already been called.
    run_loop_.Run();
    EXPECT_TRUE(failure_reason_.has_value());
    return failure_reason_.value_or(ContentVerifyJob::FAILURE_REASON_MAX);
  }

 private:
  base::RunLoop run_loop_;
  std::string extension_id_;
  base::FilePath relative_path_;
  base::Optional<ContentVerifyJob::FailureReason> failure_reason_;

  DISALLOW_COPY_AND_ASSIGN(JobTestObserver);
};

}  // namespace

class ContentVerifyJobUnittest : public ExtensionsTest {
 public:
  ContentVerifyJobUnittest()
      // The TestBrowserThreadBundle is needed for ContentVerifyJob::Start().
      : ExtensionsTest(std::make_unique<content::TestBrowserThreadBundle>(
            content::TestBrowserThreadBundle::REAL_IO_THREAD)) {}
  ~ContentVerifyJobUnittest() override {}

  // Helper to get files from our subdirectory in the general extensions test
  // data dir.
  base::FilePath GetTestPath(const base::FilePath& relative_path) {
    base::FilePath base_path;
    EXPECT_TRUE(PathService::Get(DIR_TEST_DATA, &base_path));
    base_path = base_path.AppendASCII("content_hash_fetcher");
    return base_path.Append(relative_path);
  }

  // Unzips the extension source from |extension_zip| into a temporary
  // directory and loads it. Returns the resuling Extension object.
  // |destination| points to the path where the extension was extracted.
  scoped_refptr<Extension> UnzipToTempDirAndLoad(
      const base::FilePath& extension_zip,
      base::FilePath* destination) {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    *destination = temp_dir_.GetPath();
    EXPECT_TRUE(zip::Unzip(extension_zip, *destination));

    std::string error;
    scoped_refptr<Extension> extension = file_util::LoadExtension(
        *destination, Manifest::INTERNAL, 0 /* flags */, &error);
    EXPECT_NE(nullptr, extension.get()) << " error:'" << error << "'";
    return extension;
  }

 protected:
  ContentVerifyJob::FailureReason RunContentVerifyJob(
      const Extension& extension,
      const base::FilePath& resource_path,
      std::string& resource_contents,
      ContentVerifyJobAsyncRunMode run_mode) {
    JobTestObserver observer(extension.id(), resource_path);
    scoped_refptr<ContentHashReader> content_hash_reader =
        CreateContentHashReader(extension, resource_path);
    const bool needs_to_observe_hashes_ready =
        run_mode == kHashesReadyBeforeContentRead;
    scoped_refptr<TestContentVerifyJob> verify_job = new TestContentVerifyJob(
        needs_to_observe_hashes_ready, content_hash_reader.get(),
        base::BindOnce(&DoNothingWithReasonParam));

    auto run_content_read_step = [](ContentVerifyJob* verify_job,
                                    std::string* resource_contents) {
      // Simulate serving |resource_contents| from |resource_path|.
      verify_job->BytesRead(resource_contents->size(),
                            base::string_as_array(resource_contents));
      verify_job->DoneReading();
    };

    switch (run_mode) {
      case kNone:
        verify_job->Start();  // Read hashes asynchronously.
        run_content_read_step(verify_job.get(), &resource_contents);
        break;
      case kContentReadBeforeHashesReady:
        run_content_read_step(verify_job.get(), &resource_contents);
        verify_job->Start();  // Read hashes asynchronously.
        break;
      case kHashesReadyBeforeContentRead:
        verify_job->Start();
        // Wait for hashes to become ready.
        verify_job->WaitForOnHashesReady();
        run_content_read_step(verify_job.get(), &resource_contents);
        break;
    };
    return observer.WaitAndGetFailureReason();
  }

  ContentVerifyJob::FailureReason RunContentVerifyJob(
      const Extension& extension,
      const base::FilePath& resource_path,
      std::string& resource_contents) {
    return RunContentVerifyJob(extension, resource_path, resource_contents,
                               kNone);
  }

 protected:
  base::ScopedTempDir temp_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentVerifyJobUnittest);
};

// Tests that deleted legitimate files trigger content verification failure.
// Also tests that non-existent file request does not trigger content
// verification failure.
TEST_F(ContentVerifyJobUnittest, DeletedAndMissingFiles) {
  base::FilePath unzipped_path;
  base::FilePath test_dir_base =
      GetTestPath(base::FilePath(FILE_PATH_LITERAL("with_verified_contents")));
  scoped_refptr<Extension> extension = UnzipToTempDirAndLoad(
      test_dir_base.AppendASCII("source_all.zip"), &unzipped_path);
  ASSERT_TRUE(extension.get());
  // Make sure there is a verified_contents.json file there as this test cannot
  // fetch it.
  EXPECT_TRUE(
      base::PathExists(file_util::GetVerifiedContentsPath(extension->path())));

  const base::FilePath::CharType kExistentResource[] =
      FILE_PATH_LITERAL("background.js");
  base::FilePath existent_resource_path(kExistentResource);
  {
    // Make sure background.js passes verification correctly.
    std::string contents;
    base::ReadFileToString(
        unzipped_path.Append(base::FilePath(kExistentResource)), &contents);
    EXPECT_EQ(ContentVerifyJob::NONE,
              RunContentVerifyJob(*extension.get(), existent_resource_path,
                                  contents));
  }

  {
    // Once background.js is deleted, verification will result in HASH_MISMATCH.
    // Delete the existent file first.
    EXPECT_TRUE(base::DeleteFile(
        unzipped_path.Append(base::FilePath(kExistentResource)), false));

    // Deleted file will serve empty contents.
    std::string empty_contents;
    EXPECT_EQ(ContentVerifyJob::HASH_MISMATCH,
              RunContentVerifyJob(*extension.get(), existent_resource_path,
                                  empty_contents));
  }

  {
    // Now ask for a non-existent resource non-existent.js. Verification should
    // skip this file as it is not listed in our verified_contents.json file.
    const base::FilePath::CharType kNonExistentResource[] =
        FILE_PATH_LITERAL("non-existent.js");
    base::FilePath non_existent_resource_path(kNonExistentResource);
    // Non-existent file will serve empty contents.
    std::string empty_contents;
    EXPECT_EQ(ContentVerifyJob::NONE,
              RunContentVerifyJob(*extension.get(), non_existent_resource_path,
                                  empty_contents));
  }

  {
    // Now create a resource foo.js which exists on disk but is not in the
    // extension's verified_contents.json. Verification should result in
    // NO_HASHES_FOR_FILE since the extension is trying to load a file the
    // extension should not have.
    const base::FilePath::CharType kUnexpectedResource[] =
        FILE_PATH_LITERAL("foo.js");
    base::FilePath unexpected_resource_path(kUnexpectedResource);

    base::FilePath full_path =
        unzipped_path.Append(base::FilePath(unexpected_resource_path));
    base::WriteFile(full_path, "42", sizeof("42"));

    std::string contents;
    base::ReadFileToString(full_path, &contents);
    EXPECT_EQ(ContentVerifyJob::NO_HASHES_FOR_FILE,
              RunContentVerifyJob(*extension.get(), unexpected_resource_path,
                                  contents));
  }

  {
    // Ask for the root path of the extension (i.e., chrome-extension://<id>/).
    // Verification should skip this request as if the resource were
    // non-existent. See https://crbug.com/791929.
    base::FilePath empty_path_resource_path(FILE_PATH_LITERAL(""));
    std::string empty_contents;
    EXPECT_EQ(ContentVerifyJob::NONE,
              RunContentVerifyJob(*extension.get(), empty_path_resource_path,
                                  empty_contents));
  }
}

// Tests that extension resources that are originally 0 byte behave correctly
// with content verification.
TEST_F(ContentVerifyJobUnittest, LegitimateZeroByteFile) {
  base::FilePath unzipped_path;
  base::FilePath test_dir_base =
      GetTestPath(base::FilePath(FILE_PATH_LITERAL("zero_byte_file")));
  // |extension| has a 0 byte background.js file in it.
  scoped_refptr<Extension> extension = UnzipToTempDirAndLoad(
      test_dir_base.AppendASCII("source.zip"), &unzipped_path);
  ASSERT_TRUE(extension.get());
  // Make sure there is a verified_contents.json file there as this test cannot
  // fetch it.
  EXPECT_TRUE(
      base::PathExists(file_util::GetVerifiedContentsPath(extension->path())));

  const base::FilePath::CharType kResource[] =
      FILE_PATH_LITERAL("background.js");
  base::FilePath resource_path(kResource);
  {
    // Make sure 0 byte background.js passes content verification.
    std::string contents;
    base::ReadFileToString(unzipped_path.Append(base::FilePath(kResource)),
                           &contents);
    EXPECT_EQ(ContentVerifyJob::NONE,
              RunContentVerifyJob(*extension.get(), resource_path, contents));
  }

  {
    // Make sure non-empty background.js fails content verification.
    std::string modified_contents = "console.log('non empty');";
    EXPECT_EQ(ContentVerifyJob::HASH_MISMATCH,
              RunContentVerifyJob(*extension.get(), resource_path,
                                  modified_contents));
  }
}

// Tests that extension resources of different interesting sizes work properly.
// Regression test for https://crbug.com/720597, where content verification
// always failed for sizes multiple of content hash's block size (4096 bytes).
TEST_F(ContentVerifyJobUnittest, DifferentSizedFiles) {
  base::FilePath test_dir_base =
      GetTestPath(base::FilePath(FILE_PATH_LITERAL("different_sized_files")));
  base::FilePath unzipped_path;
  scoped_refptr<Extension> extension = UnzipToTempDirAndLoad(
      test_dir_base.AppendASCII("source.zip"), &unzipped_path);
  ASSERT_TRUE(extension.get());
  // Make sure there is a verified_contents.json file there as this test cannot
  // fetch it.
  EXPECT_TRUE(
      base::PathExists(file_util::GetVerifiedContentsPath(extension->path())));

  const struct {
    const char* name;
    size_t byte_size;
  } kFilesToTest[] = {
      {"1024.js", 1024}, {"4096.js", 4096}, {"8192.js", 8192},
      {"8191.js", 8191}, {"8193.js", 8193},
  };
  for (const auto& file_to_test : kFilesToTest) {
    base::FilePath resource_path =
        base::FilePath::FromUTF8Unsafe(file_to_test.name);
    std::string contents;
    base::ReadFileToString(unzipped_path.AppendASCII(file_to_test.name),
                           &contents);
    EXPECT_EQ(file_to_test.byte_size, contents.size());
    EXPECT_EQ(ContentVerifyJob::NONE,
              RunContentVerifyJob(*extension.get(), resource_path, contents));
  }
}

class ContentMismatchUnittest
    : public ContentVerifyJobUnittest,
      public testing::WithParamInterface<ContentVerifyJobAsyncRunMode> {
 public:
  ContentMismatchUnittest() {}

 protected:
  // Runs test to verify that a modified extension resource (background.js)
  // causes ContentVerifyJob to fail with HASH_MISMATCH. The string
  // |content_to_append_for_mismatch| is appended to the resource for
  // modification. The asynchronous nature of ContentVerifyJob can be controlled
  // by |run_mode|.
  void RunContentMismatchTest(const std::string& content_to_append_for_mismatch,
                              ContentVerifyJobAsyncRunMode run_mode) {
    base::FilePath unzipped_path;
    base::FilePath test_dir_base = GetTestPath(
        base::FilePath(FILE_PATH_LITERAL("with_verified_contents")));
    scoped_refptr<Extension> extension = UnzipToTempDirAndLoad(
        test_dir_base.AppendASCII("source_all.zip"), &unzipped_path);
    ASSERT_TRUE(extension.get());
    // Make sure there is a verified_contents.json file there as this test
    // cannot fetch it.
    EXPECT_TRUE(base::PathExists(
        file_util::GetVerifiedContentsPath(extension->path())));

    const base::FilePath::CharType kResource[] =
        FILE_PATH_LITERAL("background.js");
    base::FilePath existent_resource_path(kResource);
    {
      // Make sure modified background.js fails content verification.
      std::string modified_contents;
      base::ReadFileToString(unzipped_path.Append(base::FilePath(kResource)),
                             &modified_contents);
      modified_contents.append(content_to_append_for_mismatch);
      EXPECT_EQ(ContentVerifyJob::HASH_MISMATCH,
                RunContentVerifyJob(*extension.get(), existent_resource_path,
                                    modified_contents, run_mode));
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentMismatchUnittest);
};

INSTANTIATE_TEST_CASE_P(ContentVerifyJobUnittest,
                        ContentMismatchUnittest,
                        testing::Values(kNone,
                                        kContentReadBeforeHashesReady,
                                        kHashesReadyBeforeContentRead));

// Tests that content modification causes content verification failure.
TEST_P(ContentMismatchUnittest, ContentMismatch) {
  RunContentMismatchTest("console.log('modified');", GetParam());
}

// Similar to ContentMismatch, but uses a file size > 4k.
// Regression test for https://crbug.com/804630.
TEST_P(ContentMismatchUnittest, ContentMismatchWithLargeFile) {
  std::string content_larger_than_block_size(
      extension_misc::kContentVerificationDefaultBlockSize + 1, ';');
  RunContentMismatchTest(content_larger_than_block_size, GetParam());
}

}  // namespace extensions
