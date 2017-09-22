// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_registrar.h"

#include <memory>
#include <utility>

#include "base/location.h"
#include "base/macros.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/browser/test_extension_registrar_delegate.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/browser/test_extensions_browser_client.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"

namespace extensions {

namespace {

class TestExtensionSystem : public MockExtensionSystem {
 public:
  explicit TestExtensionSystem(content::BrowserContext* context)
      : MockExtensionSystem(context),
        runtime_data_(ExtensionRegistry::Get(context)) {}

  ~TestExtensionSystem() override {}

  // MockExtensionSystem:
  void RegisterExtensionWithRequestContexts(
      const Extension* extension,
      const base::Closure& callback) override {
    base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE, callback);
  }
  RuntimeData* runtime_data() override { return &runtime_data_; }

 private:
  RuntimeData runtime_data_;
  DISALLOW_COPY_AND_ASSIGN(TestExtensionSystem);
};


}  // namespace

class ExtensionRegistrarTest : public ExtensionsTest {
 public:
  ExtensionRegistrarTest()
      : ExtensionsTest(std::make_unique<content::TestBrowserThreadBundle>()) {}
  ~ExtensionRegistrarTest() override = default;

  void SetUp() override {
    ExtensionsTest::SetUp();
    extensions_browser_client()->set_extension_system_factory(&factory_);
  }

 private:
  MockExtensionSystemFactory<TestExtensionSystem> factory_;
};

// Adds and removes an extension.
TEST_F(ExtensionRegistrarTest, Basic) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());

  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension").Build();
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>(browser_context()));

  registrar.AddExtension(extension.get());
  EXPECT_TRUE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));

  TestExtensionRegistryObserver observer(extension_registry);
  observer.WaitForExtensionReady();
  EXPECT_TRUE(extension_registry->ready_extensions().Contains(extension->id()));

  // Calling DeactivateAndRemoveExtension removes its entry from the disabled
  // list and then removes the extension.
  registrar.DeactivateAndRemoveExtension(extension->id(),
                                         UnloadedExtensionReason::UNINSTALL);
  EXPECT_FALSE(
      extension_registry->disabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));
}

// Disables an extension before removing it.
TEST_F(ExtensionRegistrarTest, DisableAndRemove) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());

  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension").Build();
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>(browser_context()));

  registrar.AddExtension(extension.get());
  EXPECT_TRUE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));

  TestExtensionRegistryObserver observer(extension_registry);
  observer.WaitForExtensionReady();
  EXPECT_TRUE(extension_registry->ready_extensions().Contains(extension->id()));

  // Disable the extension before removing it.
  registrar.DisableExtension(
      extension->id(), disable_reason::DISABLE_USER_ACTION);
  EXPECT_TRUE(
      extension_registry->disabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));

  // We can still call DeactivateAndRemoveExtension to remove its entry from
  // the disabled list and remove the extension.
  registrar.DeactivateAndRemoveExtension(extension->id(),
                                         UnloadedExtensionReason::UNINSTALL);
  EXPECT_FALSE(
      extension_registry->disabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));
}

// Adds, disables and re-enables an extension.
TEST_F(ExtensionRegistrarTest, DisableAndEnable) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());

  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension").Build();
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>(browser_context()));

  registrar.AddExtension(extension.get());
  EXPECT_TRUE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));

  {
    TestExtensionRegistryObserver observer(extension_registry);
    observer.WaitForExtensionReady();
  }
  EXPECT_TRUE(extension_registry->ready_extensions().Contains(extension->id()));

  // Disable then enable the extension.
  registrar.DisableExtension(
      extension->id(), disable_reason::DISABLE_USER_ACTION);
  EXPECT_TRUE(
      extension_registry->disabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));

  registrar.EnableExtension(extension->id());
  EXPECT_TRUE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));

  {
    TestExtensionRegistryObserver observer(extension_registry);
    observer.WaitForExtensionReady();
  }

  registrar.DeactivateAndRemoveExtension(extension->id(),
                                         UnloadedExtensionReason::UNINSTALL);
  EXPECT_FALSE(
      extension_registry->disabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->enabled_extensions().Contains(extension->id()));
  EXPECT_FALSE(
      extension_registry->ready_extensions().Contains(extension->id()));
}

// TODO: add disabled extension

}  // namespace extensions
