// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_loader.h"

#include <memory>
#include <utility>

#include "base/location.h"
#include "base/macros.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/browser/test_extensions_browser_client.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"

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
      : ExtensionsTest(std::make_unique<content::TestBrowserThreadBundle>()) {}
  ~ExtensionLoaderTest() override = default;

  void SetUp() override {
    ExtensionsTest::SetUp();
    extensions_browser_client()->set_extension_system_factory(&factory_);
  }

 private:
  MockExtensionSystemFactory<TestExtensionSystem> factory_;
};

// Loads and unloads an extension.
TEST_F(ExtensionLoaderTest, NotifyExtensionLoadedUnloaded) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());

  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension").Build();
  ExtensionLoader loader(browser_context());
  loader.NotifyExtensionLoaded(extension);

  EXPECT_TRUE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));

  TestExtensionRegistryObserver observer(extension_registry);
  observer.WaitForExtensionReady();
  EXPECT_TRUE(extension_registry->ready_extensions().Contains(extension->id()));

  loader.NotifyExtensionUnloaded(extension, UnloadedExtensionReason::DISABLE);

  EXPECT_FALSE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));
}

}  // namespace extensions
