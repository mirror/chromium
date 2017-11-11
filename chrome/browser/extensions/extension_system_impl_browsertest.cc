// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_system_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/extension_install_prompt_show_params.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/pending_extension_manager.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension_builder.h"

namespace extensions {

namespace {

bool IsExtension(const Extension* extension) {
  return extension->GetType() == Manifest::TYPE_EXTENSION;
}

class ExtensionSystemImplTest : public ExtensionBrowserTest {
 protected:
  ExtensionSystemImpl* extension_system() const {
    return static_cast<ExtensionSystemImpl*>(
        extensions::ExtensionSystem::Get(browser()->profile()));
  }

  void RunInstallUpdate(const std::string& extension_id,
                        const std::string public_key,
                        const base::FilePath& unpacked_dir,
                        ExtensionSystem::InstallUpdateCallback callback) {
    ExtensionSystemImpl* system = extension_system();
    ASSERT_TRUE(system != nullptr);
    bool executed = false;

    base::RunLoop run_loop;

    system->InstallUpdate(
        extension_id, public_key, unpacked_dir,
        base::BindOnce(
            [](bool* executed, const base::Closure& quit_closure,
               ExtensionSystem::InstallUpdateCallback callback, bool success) {
              *executed = true;
              std::move(callback).Run(success);
              quit_closure.Run();
            },
            &executed, run_loop.QuitWhenIdleClosure(), std::move(callback)));
    content::RunThisRunLoop(&run_loop);

    ASSERT_TRUE(executed);
  }

  const Extension* GetInstalledExtension(
      const std::string& extension_id) const {
    return extension_system()->extension_service()->GetInstalledExtension(
        extension_id);
  }

  base::FilePath UnpackedCrxTempDir() const {
    base::ScopedTempDir temp_dir;
    EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
    EXPECT_TRUE(base::PathExists(temp_dir.GetPath()));

    base::FilePath unpacked_path = test_data_dir_.AppendASCII("good_unpacked");
    EXPECT_TRUE(base::PathExists(unpacked_path));
    EXPECT_TRUE(base::CopyDirectory(unpacked_path, temp_dir.GetPath(), false));

    return temp_dir.Take();
  }

  // Helper function that creates a file at |relative_path| within |directory|
  // and fills it with |content|.
  bool AddFileToDirectory(const base::FilePath& directory,
                          const base::FilePath& relative_path,
                          const std::string& content) const {
    base::FilePath full_path = directory.Append(relative_path);
    if (!CreateDirectory(full_path.DirName()))
      return false;
    int result = base::WriteFile(full_path, content.data(), content.size());
    return (static_cast<size_t>(result) == content.size());
  }

  void AddExtension(const std::string& extension_id,
                    const std::string& version) const {
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
    ASSERT_TRUE(base::PathExists(temp_dir.GetPath()));

    base::FilePath foo_js(FILE_PATH_LITERAL("foo.js"));
    base::FilePath bar_html(FILE_PATH_LITERAL("bar/bar.html"));
    ASSERT_TRUE(AddFileToDirectory(temp_dir.GetPath(), foo_js, "hello"))
        << "Failed to write " << temp_dir.GetPath().value() << "/"
        << foo_js.value();
    ASSERT_TRUE(AddFileToDirectory(temp_dir.GetPath(), bar_html, "world"));

    ExtensionBuilder builder;
    builder.SetManifest(DictionaryBuilder()
                            .Set("name", "My First Extension")
                            .Set("version", version)
                            .Set("manifest_version", 2)
                            .Build());
    builder.SetID(extension_id);
    builder.SetPath(temp_dir.GetPath());
    ExtensionRegistry::Get(browser()->profile())->AddEnabled(builder.Build());

    const Extension* extension = GetInstalledExtension(kExtensionId);
    ASSERT_NE(nullptr, extension);
    ASSERT_EQ(version, extension->VersionString());
  }

  void AddExtensionToPending(const std::string& extension_id) const {
    ASSERT_TRUE(extension_system()
                    ->extension_service()
                    ->pending_extension_manager()
                    ->AddFromSync(kExtensionId, kFakeUpdateURL, base::Version(),
                                  &IsExtension, kFakeRemoteInstall));
  }

  const GURL kFakeUpdateURL = GURL("http://fake.update/url");
  const bool kFakeRemoteInstall = false;
  const std::string kExtensionId = "ldnnhddmnhbkjipkidpdiheffobcpfmf";
  const std::string kPublicKey =
      "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC8c4fBSPZ6utYoZ8NiWF/"
      "DSaimBhihjwgOsskyleFGaurhi3TDClTVSGPxNkgCzrz0wACML7M4aNjpd05qupdbR2d294j"
      "kDuI7caxEGUucpP7GJRRHnm8Sx+"
      "y0ury28n8jbN0PnInKKWcxpIXXmNQyC19HBuO3QIeUq9Dqc+7YFQIDAQAB";
};

IN_PROC_BROWSER_TEST_F(ExtensionSystemImplTest, InstallUpdate_NewExtension) {
  base::ScopedAllowBlockingForTesting allow_io;

  // InstallUpdate will do nothing if an extension is new.
  ASSERT_EQ(nullptr, GetInstalledExtension(kExtensionId));
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.Set(UnpackedCrxTempDir()));
  RunInstallUpdate(kExtensionId, kPublicKey, temp_dir.GetPath(),
                   base::BindOnce([](bool success) { EXPECT_FALSE(success); }));
  // The unpacked folder should be deleted.
  EXPECT_FALSE(base::PathExists(temp_dir.GetPath()));
  EXPECT_EQ(nullptr, GetInstalledExtension(kExtensionId));
}

IN_PROC_BROWSER_TEST_F(ExtensionSystemImplTest,
                       InstallUpdate_UpdateExistingExtension) {
  base::ScopedAllowBlockingForTesting allow_io;

  // This test is to try to update an existing extension that's not
  // in the pending list.
  AddExtension(kExtensionId, "0.0");

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.Set(UnpackedCrxTempDir()));
  RunInstallUpdate(kExtensionId, kPublicKey, temp_dir.GetPath(),
                   base::BindOnce([](bool success) { EXPECT_TRUE(success); }));
  // The unpacked folder should be deleted.
  EXPECT_FALSE(base::PathExists(temp_dir.GetPath()));

  const Extension* extension = GetInstalledExtension(kExtensionId);
  ASSERT_NE(nullptr, extension);
  EXPECT_EQ("1.0", extension->VersionString());
}

IN_PROC_BROWSER_TEST_F(ExtensionSystemImplTest,
                       InstallUpdate_InvalidPublicKey) {
  base::ScopedAllowBlockingForTesting allow_io;

  // There will be no update because the public key is not valid.
  AddExtension(kExtensionId, "0.0");

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.Set(UnpackedCrxTempDir()));
  RunInstallUpdate(kExtensionId, "invalid public key", temp_dir.GetPath(),
                   base::BindOnce([](bool success) { EXPECT_FALSE(success); }));
  // The unpacked folder should be deleted.
  EXPECT_FALSE(base::PathExists(temp_dir.GetPath()));

  const Extension* extension = GetInstalledExtension(kExtensionId);
  ASSERT_NE(nullptr, extension);
  EXPECT_EQ("0.0", extension->VersionString());
}

IN_PROC_BROWSER_TEST_F(ExtensionSystemImplTest,
                       InstallUpdate_UpdatePendingExtension) {
  base::ScopedAllowBlockingForTesting allow_io;

  // Add the extension id to the pending list.
  AddExtensionToPending(kExtensionId);
  ASSERT_EQ(nullptr, GetInstalledExtension(kExtensionId));

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.Set(UnpackedCrxTempDir()));
  RunInstallUpdate(kExtensionId, kPublicKey, temp_dir.GetPath(),
                   base::BindOnce([](bool success) { EXPECT_FALSE(success); }));
  // The unpacked folder should be deleted.
  EXPECT_FALSE(base::PathExists(temp_dir.GetPath()));

  const Extension* extension = GetInstalledExtension(kExtensionId);
  ASSERT_EQ(nullptr, extension);
}

IN_PROC_BROWSER_TEST_F(ExtensionSystemImplTest,
                       InstallUpdate_UpdateExistingAndPendingExtension) {
  // Update an existing extension that is in the pending list.
  base::ScopedAllowBlockingForTesting allow_io;

  // This test is to try to update an existing extension that's not
  // in the pending list.
  AddExtensionToPending(kExtensionId);
  AddExtension(kExtensionId, "0.0");

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.Set(UnpackedCrxTempDir()));
  RunInstallUpdate(kExtensionId, kPublicKey, temp_dir.GetPath(),
                   base::BindOnce([](bool success) { EXPECT_TRUE(success); }));
  // The unpacked folder should be deleted.
  EXPECT_FALSE(base::PathExists(temp_dir.GetPath()));

  const Extension* extension = GetInstalledExtension(kExtensionId);
  ASSERT_NE(nullptr, extension);
  EXPECT_EQ("1.0", extension->VersionString());
}

}  // namespace

}  // namespace extensions
