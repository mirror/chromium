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
#include "content/public/test/test_notification_tracker.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/notification_types.h"
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
    extension_ = ExtensionBuilder("extension").Build();
  }

 protected:
  // Verifies that the extentsion is in the given set in the ExtensionRegistry
  // and not in other sets.
  void ExpectInSet(ExtensionRegistry::IncludeFlag set_id) {
    ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context());

    bool expect_in_enabled = set_id & ExtensionRegistry::ENABLED;
    EXPECT_EQ(expect_in_enabled,
              registry->enabled_extensions().Contains(extension_->id()));

    bool expect_in_disabled = set_id & ExtensionRegistry::DISABLED;
    EXPECT_EQ(expect_in_disabled,
              registry->disabled_extensions().Contains(extension_->id()));

    bool expect_in_terminated = set_id & ExtensionRegistry::TERMINATED;
    EXPECT_EQ(expect_in_terminated,
              registry->terminated_extensions().Contains(extension_->id()));

    bool expect_in_blacklisted = set_id & ExtensionRegistry::BLACKLISTED;
    EXPECT_EQ(expect_in_blacklisted,
              registry->blacklisted_extensions().Contains(extension_->id()));

    bool expect_in_blocked = set_id & ExtensionRegistry::BLOCKED;
    EXPECT_EQ(expect_in_blocked,
              registry->blocked_extensions().Contains(extension_->id()));
  }

  bool IsExtensionReady() {
    return ExtensionRegistry::Get(browser_context())
        ->ready_extensions()
        .Contains(extension_->id());
  }

  scoped_refptr<const Extension> extension() const { return extension_; }

 private:
  MockExtensionSystemFactory<TestExtensionSystem> factory_;
  scoped_refptr<const Extension> extension_;
};

// Adds and removes an extension.
TEST_F(ExtensionRegistrarTest, Basic) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>());

  registrar.AddExtension(extension(),
                         ExtensionRegistrar::InitialState::ENABLED);
  ExpectInSet(ExtensionRegistry::ENABLED);
  EXPECT_FALSE(IsExtensionReady());

  TestExtensionRegistryObserver observer(extension_registry);
  observer.WaitForExtensionReady();
  EXPECT_TRUE(IsExtensionReady());

  // Removing the extension should trigger the notification.
  content::TestNotificationTracker notification_tracker;
  notification_tracker.ListenFor(
      extensions::NOTIFICATION_EXTENSION_REMOVED,
      content::Source<content::BrowserContext>(browser_context()));

  // Calling RemoveExtension removes its entry from the enabled list and then
  // removes the extension.
  registrar.RemoveExtension(extension()->id(),
                            UnloadedExtensionReason::UNINSTALL);
  ExpectInSet(ExtensionRegistry::NONE);
  notification_tracker.Check1AndReset(
      extensions::NOTIFICATION_EXTENSION_REMOVED);
}

// Disables an extension before removing it.
TEST_F(ExtensionRegistrarTest, DisableAndRemove) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>());

  registrar.AddExtension(extension(),
                         ExtensionRegistrar::InitialState::ENABLED);
  ExpectInSet(ExtensionRegistry::ENABLED);
  EXPECT_FALSE(IsExtensionReady());

  TestExtensionRegistryObserver observer(extension_registry);
  observer.WaitForExtensionReady();
  EXPECT_TRUE(IsExtensionReady());

  // Disable the extension before removing it.
  registrar.DisableExtension(extension()->id(),
                             disable_reason::DISABLE_USER_ACTION);
  ExpectInSet(ExtensionRegistry::DISABLED);
  EXPECT_FALSE(IsExtensionReady());

  // Removing the disabled extension should still trigger this notification.
  content::TestNotificationTracker notification_tracker;
  notification_tracker.ListenFor(
      extensions::NOTIFICATION_EXTENSION_REMOVED,
      content::Source<content::BrowserContext>(browser_context()));

  registrar.RemoveExtension(extension()->id(),
                            UnloadedExtensionReason::UNINSTALL);
  ExpectInSet(ExtensionRegistry::NONE);
  notification_tracker.Check1AndReset(
      extensions::NOTIFICATION_EXTENSION_REMOVED);
}

// Adds, disables and re-enables an extension.
TEST_F(ExtensionRegistrarTest, DisableAndEnable) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>());

  registrar.AddExtension(extension(),
                         ExtensionRegistrar::InitialState::ENABLED);
  ExpectInSet(ExtensionRegistry::ENABLED);
  EXPECT_FALSE(IsExtensionReady());

  {
    TestExtensionRegistryObserver observer(extension_registry);
    observer.WaitForExtensionReady();
  }
  EXPECT_TRUE(IsExtensionReady());

  // Disable then enable the extension.
  registrar.DisableExtension(extension()->id(),
                             disable_reason::DISABLE_USER_ACTION);
  ExpectInSet(ExtensionRegistry::DISABLED);
  EXPECT_FALSE(IsExtensionReady());

  registrar.EnableExtension(extension()->id());
  ExpectInSet(ExtensionRegistry::ENABLED);
  EXPECT_FALSE(IsExtensionReady());

  {
    TestExtensionRegistryObserver observer(extension_registry);
    observer.WaitForExtensionReady();
  }
  ExpectInSet(ExtensionRegistry::ENABLED);
  EXPECT_TRUE(IsExtensionReady());

  registrar.RemoveExtension(extension()->id(),
                            UnloadedExtensionReason::UNINSTALL);
  ExpectInSet(ExtensionRegistry::NONE);
  EXPECT_FALSE(IsExtensionReady());
}

// Adds a disabled extension.
TEST_F(ExtensionRegistrarTest, AddDisabled) {
  ExtensionRegistry* extension_registry =
      ExtensionRegistry::Get(browser_context());
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>());

  content::TestNotificationTracker notification_tracker;
  notification_tracker.ListenFor(
      extensions::NOTIFICATION_EXTENSION_UPDATE_DISABLED,
      content::Source<content::BrowserContext>(browser_context()));
  notification_tracker.ListenFor(
      extensions::NOTIFICATION_EXTENSION_REMOVED,
      content::Source<content::BrowserContext>(browser_context()));

  // An extension can be added as disabled, then removed.
  registrar.AddExtension(extension(),
                         ExtensionRegistrar::InitialState::DISABLED);
  ExpectInSet(ExtensionRegistry::DISABLED);

  registrar.RemoveExtension(extension()->id(),
                            UnloadedExtensionReason::UNINSTALL);
  ExpectInSet(ExtensionRegistry::NONE);

  // An extension can be added as disabled, then enabled.
  registrar.AddExtension(extension(),
                         ExtensionRegistrar::InitialState::DISABLED);
  ExpectInSet(ExtensionRegistry::DISABLED);
  EXPECT_FALSE(IsExtensionReady());

  registrar.EnableExtension(extension()->id());
  ExpectInSet(ExtensionRegistry::ENABLED);
  {
    TestExtensionRegistryObserver observer(extension_registry);
    observer.WaitForExtensionReady();
  }
  EXPECT_TRUE(IsExtensionReady());

  registrar.RemoveExtension(extension()->id(),
                            UnloadedExtensionReason::UNINSTALL);
  ExpectInSet(ExtensionRegistry::NONE);
  EXPECT_FALSE(IsExtensionReady());

  notification_tracker.Check2AndReset(
      extensions::NOTIFICATION_EXTENSION_UPDATE_DISABLED,
      extensions::NOTIFICATION_EXTENSION_REMOVED);
}

// Adds a blacklisted extension.
TEST_F(ExtensionRegistrarTest, AddBlacklisted) {
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>());

  // An extension can be added as blacklisted.
  registrar.AddExtension(extension(),
                         ExtensionRegistrar::InitialState::BLACKLISTED);
  ExpectInSet(ExtensionRegistry::BLACKLISTED);

  // RemoveExtension does not un-blacklist the extension.
  registrar.RemoveExtension(extension()->id(),
                            UnloadedExtensionReason::BLACKLIST);
  ExpectInSet(ExtensionRegistry::BLACKLISTED);
}

// Adds a blocked extension.
TEST_F(ExtensionRegistrarTest, AddBlocked) {
  ExtensionRegistrar registrar(
      browser_context(), ExtensionPrefs::Get(browser_context()),
      std::make_unique<TestExtensionRegistrarDelegate>());

  // An extension can be added as blocked.
  registrar.AddExtension(extension(),
                         ExtensionRegistrar::InitialState::BLOCKED);
  ExpectInSet(ExtensionRegistry::BLOCKED);

  // RemoveExtension does not un-block the extension.
  registrar.RemoveExtension(extension()->id(),
                            UnloadedExtensionReason::LOCK_ALL);
  ExpectInSet(ExtensionRegistry::BLOCKED);
}

// TODO: Test Notify (see ExtensionService tests)

}  // namespace extensions
