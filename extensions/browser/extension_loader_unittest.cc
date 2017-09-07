// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_loader.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/browser/test_extensions_browser_client.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_paths.h"

namespace extensions {

namespace {

class TestExtensionSystem : public MockExtensionSystem {
 public:
  explicit TestExtensionSystem(content::BrowserContext* context)
      : MockExtensionSystem(context) {}

  ~TestExtensionSystem() override {}

  void RegisterExtensionWithRequestContexts(
      const Extension* extension,
      const base::Closure& callback) override {
    base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE, callback);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestExtensionSystem);
};

}  // namespace

class ExtensionLoaderTest : public ExtensionsTest {
 public:
  ExtensionLoaderTest()
      : ExtensionsTest(std::make_unique<content::TestBrowserThreadBundle>()) {
    PathService::Get(extensions::DIR_TEST_DATA, &test_data_dir_);
  }

  ~ExtensionLoaderTest() override {}

  void SetUp() override {
    ExtensionsTest::SetUp();

    user_prefs::UserPrefs::Set(browser_context(), &testing_pref_service_);
    extensions_browser_client()->set_extension_system_factory(&factory_);
  }

 protected:
  // Waits for the enabled extension to become loaded and ready.
  void TestLoadExtension(scoped_refptr<const Extension> extension) {
    EXPECT_TRUE(extension.get());
    EXPECT_TRUE(
        extension_registry()->enabled_extensions().Contains(extension->id()));
    EXPECT_FALSE(
        extension_registry()->ready_extensions().Contains(extension->id()));

    TestExtensionRegistryObserver observer(extension_registry());
    observer.WaitForExtensionLoaded();
    observer.WaitForExtensionReady();
    EXPECT_TRUE(
        extension_registry()->ready_extensions().Contains(extension->id()));
  }

  ExtensionRegistry* extension_registry() {
    return ExtensionRegistry::Get(browser_context());
  }

  base::FilePath bad_extension_path() const {
    return test_data_dir_.AppendASCII("file_util/bad_manifest");
  }

  base::FilePath extension_path() const {
    return test_data_dir_.AppendASCII("extension");
  }

 private:
  TestingPrefServiceSimple testing_pref_service_;
  MockExtensionSystemFactory<TestExtensionSystem> factory_;

  base::FilePath test_data_dir_;
};

// Loads an extension.
TEST_F(ExtensionLoaderTest, LoadExtension) {
  ExtensionLoader loader(browser_context());
  scoped_refptr<const Extension> extension =
      loader.LoadExtensionFromCommandLine(extension_path());
  TestLoadExtension(extension);
}

// Attempts to load an invalid extension.
TEST_F(ExtensionLoaderTest, LoadInvalidExtension) {
  ExtensionLoader loader(browser_context());
  scoped_refptr<const Extension> extension =
      loader.LoadExtensionFromCommandLine(bad_extension_path());
  EXPECT_FALSE(extension.get());
}

// Attempts to load a missing extension.
TEST_F(ExtensionLoaderTest, LoadMissingExtension) {
  ExtensionLoader loader(browser_context());
  scoped_refptr<const Extension> extension =
      loader.LoadExtensionFromCommandLine(
          extension_path().AppendASCII("non-existent-path"));
  EXPECT_FALSE(extension.get());
}

// Starts loading an extension but stops before completion. Mostly verifies that
// nothing crashes in this edge case.
TEST_F(ExtensionLoaderTest, StopLoadingExtension) {
  scoped_refptr<const Extension> extension;
  {
    // Temporary.
    ExtensionLoader loader(browser_context());
    extension = loader.LoadExtensionFromCommandLine(extension_path());

    EXPECT_TRUE(extension.get());
    TestExtensionRegistryObserver observer(extension_registry());
    observer.WaitForExtensionLoaded();

    EXPECT_TRUE(
        extension_registry()->enabled_extensions().Contains(extension->id()));
    EXPECT_FALSE(
        extension_registry()->ready_extensions().Contains(extension->id()));
  }

  // Spin and then verify the extension didn't finish loading.
  content::RunAllBlockingPoolTasksUntilIdle();
  EXPECT_TRUE(
      extension_registry()->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry()->ready_extensions().Contains(extension->id()));
}

}  // namespace extensions
