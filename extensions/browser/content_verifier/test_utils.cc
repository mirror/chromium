// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/content_verifier/test_utils.h"

#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/google/zip.h"

namespace extensions {

TestContentVerifyJobObserver::TestContentVerifyJobObserver(
    const ExtensionId& extension_id,
    const base::FilePath& relative_path)
    : extension_id_(extension_id), relative_path_(relative_path) {
  ContentVerifyJob::SetObserverForTests(this);
}

TestContentVerifyJobObserver::~TestContentVerifyJobObserver() {
  ContentVerifyJob::SetObserverForTests(nullptr);
}

void TestContentVerifyJobObserver::JobFinished(
    const ExtensionId& extension_id,
    const base::FilePath& relative_path,
    ContentVerifyJob::FailureReason reason) {
  if (extension_id != extension_id_ || relative_path != relative_path_)
    return;
  failure_reason_ = reason;
  run_loop_.Quit();
}

ContentVerifyJob::FailureReason
TestContentVerifyJobObserver::WaitAndGetFailureReason() {
  // Run() returns immediately if Quit() has already been called.
  run_loop_.Run();
  EXPECT_TRUE(failure_reason_.has_value());
  return failure_reason_.value_or(ContentVerifyJob::FAILURE_REASON_MAX);
}

scoped_refptr<Extension> UnzipToDirAndLoadExtension(
    const base::FilePath& extension_zip,
    const base::FilePath& unzip_dir) {
  EXPECT_TRUE(zip::Unzip(extension_zip, unzip_dir));
  std::string error;
  scoped_refptr<Extension> extension = file_util::LoadExtension(
      unzip_dir, Manifest::INTERNAL, 0 /* flags */, &error);
  EXPECT_NE(nullptr, extension.get()) << " error:'" << error << "'";
  return extension;
}

}  // namespace extensions
