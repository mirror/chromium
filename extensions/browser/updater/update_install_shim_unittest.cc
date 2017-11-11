// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/updater/update_install_shim.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extensions_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

using UpdateClientCallback = extensions::UpdateClientCallback;

class UpdateInstallShimTest : public ExtensionsTest {
 public:
  using Result = update_client::CrxInstaller::Result;
  using InstallError = update_client::InstallError;

  UpdateInstallShimTest();
  ~UpdateInstallShimTest() override;

  void InstallCompleteCallback(const Result& result);

 protected:
  void RunThreads();

 protected:
  const std::string kExtensionId = "ldnnhddmnhbkjipkidpdiheffobcpfmf";
  const std::string kPublicKey =
      "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC8c4fBSPZ6utYoZ8NiWF/"
      "DSaimBhihjwgOsskyleFGaurhi3TDClTVSGPxNkgCzrz0wACML7M4aNjpd05qupdbR2d294j"
      "kDuI7caxEGUucpP7GJRRHnm8Sx+"
      "y0ury28n8jbN0PnInKKWcxpIXXmNQyC19HBuO3QIeUq9Dqc+7YFQIDAQAB";

  scoped_refptr<content::MessageLoopRunner> loop_runner_;
  Result result_;
  bool executed_;

  DISALLOW_COPY_AND_ASSIGN(UpdateInstallShimTest);
};

UpdateInstallShimTest::UpdateInstallShimTest()
    : ExtensionsTest(std::make_unique<content::TestBrowserThreadBundle>()),
      loop_runner_(base::MakeRefCounted<content::MessageLoopRunner>()),
      result_(-1),
      executed_(false) {}

UpdateInstallShimTest::~UpdateInstallShimTest() {}

void UpdateInstallShimTest::InstallCompleteCallback(const Result& result) {
  result_ = result;
  executed_ = true;
  loop_runner_->Quit();
}

void UpdateInstallShimTest::RunThreads() {
  loop_runner_->Run();
}

TEST_F(UpdateInstallShimTest, GetInstalledFile) {
  base::ScopedTempDir root_dir;
  ASSERT_TRUE(root_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(root_dir.GetPath()));
  scoped_refptr<UpdateInstallShim> installer =
      base::MakeRefCounted<UpdateInstallShim>(kExtensionId, root_dir.GetPath(),
                                              UpdateInstallShimCallback());

  base::FilePath installed_file;

#ifdef FILE_PATH_USES_DRIVE_LETTERS
  const std::string absolute_path = "C:\\abc\\def";
  const std::string relative_path = "abc\\..\\def\\ghi";
#else
  const std::string absolute_path = "/abc/def";
  const std::string relative_path = "/abc/../def/ghi";
#endif

  installed_file.clear();
  EXPECT_FALSE(installer->GetInstalledFile(absolute_path, &installed_file));
  installed_file.clear();
  EXPECT_FALSE(installer->GetInstalledFile(relative_path, &installed_file));
  installed_file.clear();
  EXPECT_FALSE(installer->GetInstalledFile("extension", &installed_file));

  installed_file.clear();
  base::FilePath temp_file;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(root_dir.GetPath(), &temp_file));
  base::FilePath base_temp_file = temp_file.BaseName();
  EXPECT_TRUE(installer->GetInstalledFile(
      std::string(base_temp_file.value().begin(), base_temp_file.value().end()),
      &installed_file));
  EXPECT_EQ(temp_file, installed_file);
}

TEST_F(UpdateInstallShimTest, InvalidUnpackedDir) {
  // The unpacked folder is not valid, the installer will return an error.
  base::ScopedTempDir root_dir;
  ASSERT_TRUE(root_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(root_dir.GetPath()));
  scoped_refptr<UpdateInstallShim> installer =
      base::MakeRefCounted<UpdateInstallShim>(
          kExtensionId, root_dir.GetPath(),
          base::BindOnce([](const std::string& extension_id,
                            const std::string& public_key,
                            const base::FilePath& unpacked_dir,
                            UpdateClientCallback update_client_callback) {
            // This function should never be executed.
            EXPECT_TRUE(false);
          }));

  // Non-existing unpacked dir
  const base::FilePath unpacked_dir(FILE_PATH_LITERAL("unpacked_dir"));
  installer->Install(
      unpacked_dir, kPublicKey,
      base::BindOnce(&UpdateInstallShimTest::InstallCompleteCallback,
                     base::Unretained(this)));

  RunThreads();

  EXPECT_TRUE(executed_);
  EXPECT_EQ(static_cast<int>(InstallError::GENERIC_ERROR), result_.error);
}

TEST_F(UpdateInstallShimTest, EmptyInstallCallback) {
  // The update install callback is empty, the installer should return an error.
  base::ScopedTempDir root_dir;
  ASSERT_TRUE(root_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(root_dir.GetPath()));
  scoped_refptr<UpdateInstallShim> installer =
      base::MakeRefCounted<UpdateInstallShim>(kExtensionId, root_dir.GetPath(),
                                              UpdateInstallShimCallback());

  base::ScopedTempDir unpacked_dir;
  ASSERT_TRUE(unpacked_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(unpacked_dir.GetPath()));
  installer->Install(
      unpacked_dir.GetPath(), kPublicKey,
      base::BindOnce(&UpdateInstallShimTest::InstallCompleteCallback,
                     base::Unretained(this)));

  RunThreads();

  EXPECT_TRUE(executed_);
  EXPECT_EQ(static_cast<int>(InstallError::GENERIC_ERROR), result_.error);
}

TEST_F(UpdateInstallShimTest, BasicInstallOperation_Error) {
  base::ScopedTempDir root_dir;
  ASSERT_TRUE(root_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(root_dir.GetPath()));
  scoped_refptr<UpdateInstallShim> installer =
      base::MakeRefCounted<UpdateInstallShim>(
          kExtensionId, root_dir.GetPath(),
          base::BindOnce([](const std::string& extension_id,
                            const std::string& public_key,
                            const base::FilePath& unpacked_dir,
                            UpdateClientCallback update_client_callback) {
            std::move(update_client_callback)
                .Run(Result(InstallError::GENERIC_ERROR));
          }));

  base::ScopedTempDir unpacked_dir;
  ASSERT_TRUE(unpacked_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(unpacked_dir.GetPath()));

  installer->Install(
      unpacked_dir.GetPath(), kPublicKey,
      base::BindOnce(&UpdateInstallShimTest::InstallCompleteCallback,
                     base::Unretained(this)));

  RunThreads();

  EXPECT_TRUE(executed_);
  EXPECT_EQ(static_cast<int>(InstallError::GENERIC_ERROR), result_.error);
}

TEST_F(UpdateInstallShimTest, BasicInstallOperation_Success) {
  base::ScopedTempDir root_dir;
  ASSERT_TRUE(root_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(root_dir.GetPath()));
  scoped_refptr<UpdateInstallShim> installer =
      base::MakeRefCounted<UpdateInstallShim>(
          kExtensionId, root_dir.GetPath(),
          base::BindOnce([](const std::string& extension_id,
                            const std::string& public_key,
                            const base::FilePath& unpacked_dir,
                            UpdateClientCallback update_client_callback) {
            std::move(update_client_callback).Run(Result(InstallError::NONE));
          }));

  base::ScopedTempDir unpacked_dir;
  ASSERT_TRUE(unpacked_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::PathExists(unpacked_dir.GetPath()));

  installer->Install(
      unpacked_dir.GetPath(), kPublicKey,
      base::BindOnce(&UpdateInstallShimTest::InstallCompleteCallback,
                     base::Unretained(this)));

  RunThreads();

  EXPECT_TRUE(executed_);
  EXPECT_EQ(static_cast<int>(InstallError::NONE), result_.error);
}

}  // namespace

}  // namespace extensions
