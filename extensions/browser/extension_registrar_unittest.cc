// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_registrar.h"

#include <memory>

#include "base/location.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_notification_tracker.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/browser/test_extensions_browser_client.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

using testing::Return;

enum class ChangeType { ACTIVATED, DEACTIVATED };

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

class TestExtensionRegistrarDelegate : public ExtensionRegistrar::Delegate {
 public:
  TestExtensionRegistrarDelegate() = default;
  ~TestExtensionRegistrarDelegate() override = default;

  // ExtensionRegistrar::Delegate:
  MOCK_METHOD2(PostActivateExtension,
               void(scoped_refptr<const Extension> extension,
                    bool is_newly_added));
  MOCK_METHOD1(PostDeactivateExtension,
               void(scoped_refptr<const Extension> extension));
  MOCK_METHOD1(CanEnableExtension, bool(const Extension* extension));
  MOCK_METHOD1(CanDisableExtension, bool(const Extension* extension));
  MOCK_METHOD1(ShouldBlockExtension, bool(const Extension* extension));

 private:
  DISALLOW_COPY_AND_ASSIGN(TestExtensionRegistrarDelegate);
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
    registrar_.emplace(browser_context(), delegate());

    notification_tracker_.ListenFor(
        extensions::NOTIFICATION_EXTENSION_UPDATE_DISABLED,
        content::Source<content::BrowserContext>(browser_context()));
    notification_tracker_.ListenFor(
        extensions::NOTIFICATION_EXTENSION_REMOVED,
        content::Source<content::BrowserContext>(browser_context()));

    // Mock defaults.
    EXPECT_CALL(delegate_, CanEnableExtension(extension_.get()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(delegate_, CanDisableExtension(extension_.get()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(delegate_, ShouldBlockExtension(extension_.get()))
        .WillRepeatedly(Return(false));
  }

 protected:
  // Helper to add the extension as enabled and verify the result.
  void AddEnabledExtension() {
    ExtensionRegistry* extension_registry =
        ExtensionRegistry::Get(browser_context());

    EXPECT_CALL(delegate_, PostActivateExtension(extension_, true));
    registrar_->AddExtension(extension_);
    ExpectInSet(ExtensionRegistry::ENABLED);
    EXPECT_FALSE(IsExtensionReady());

    TestExtensionRegistryObserver(extension_registry).WaitForExtensionReady();
    EXPECT_TRUE(IsExtensionReady());
  }

  // Helper to add the extension as disabled and verify the result.
  void AddDisabledExtension() {
    ExtensionPrefs::Get(browser_context())
        ->SetExtensionDisabled(extension_->id(),
                               disable_reason::DISABLE_USER_ACTION);
    registrar_->AddExtension(extension_);
    ExpectInSet(ExtensionRegistry::DISABLED);
    EXPECT_FALSE(IsExtensionReady());
    EXPECT_TRUE(notification_tracker_.Check1AndReset(
        extensions::NOTIFICATION_EXTENSION_UPDATE_DISABLED));
  }

  // Helper to add the extension as blacklisted and verify the result.
  void AddBlacklistedExtension() {
    ExtensionPrefs::Get(browser_context())
        ->SetExtensionBlacklistState(extension_->id(), BLACKLISTED_MALWARE);
    registrar_->AddExtension(extension_);
    ExpectInSet(ExtensionRegistry::BLACKLISTED);
    EXPECT_FALSE(IsExtensionReady());
    EXPECT_EQ(0u, notification_tracker_.size());
  }

  // Helper to add the extension as blocked and verify the result.
  void AddBlockedExtension() {
    registrar_->AddExtension(extension_);
    ExpectInSet(ExtensionRegistry::BLOCKED);
    EXPECT_FALSE(IsExtensionReady());
    EXPECT_EQ(0u, notification_tracker_.size());
  }

  // Helper to remove an enabled extension and verify the result.
  void RemoveEnabledExtension() {
    // Calling RemoveExtension removes its entry from the enabled list and
    // removes the extension.
    EXPECT_CALL(delegate_, PostDeactivateExtension(extension_));
    registrar_->RemoveExtension(extension_->id(),
                                UnloadedExtensionReason::UNINSTALL);
    ExpectInSet(ExtensionRegistry::NONE);

    // Removing an enabled extension should trigger a notification.
    EXPECT_TRUE(notification_tracker_.Check1AndReset(
        extensions::NOTIFICATION_EXTENSION_REMOVED));
  }

  // Helper to remove a disabled extension and verify the result.
  void RemoveDisabledExtension() {
    // Calling RemoveExtension removes its entry from the disabled list and
    // removes the extension.
    registrar_->RemoveExtension(extension_->id(),
                                UnloadedExtensionReason::UNINSTALL);
    ExpectInSet(ExtensionRegistry::NONE);

    // Removing a disabled extension should trigger a notification.
    EXPECT_TRUE(notification_tracker_.Check1AndReset(
        extensions::NOTIFICATION_EXTENSION_REMOVED));
  }

  // Helper to remove a blacklisted extension and verify the result.
  void RemoveBlacklistedExtension() {
    // Calling RemoveExtension removes the extension.
    // TODO(michaelpg): Blacklisted extensions shouldn't need to be
    // "deactivated". See crbug.com/708230.
    EXPECT_CALL(delegate_, PostDeactivateExtension(extension_));
    registrar_->RemoveExtension(extension_->id(),
                                UnloadedExtensionReason::UNINSTALL);

    // RemoveExtension does not un-blacklist the extension.
    ExpectInSet(ExtensionRegistry::BLACKLISTED);

    // Removing a blacklisted extension should trigger a notification.
    EXPECT_TRUE(notification_tracker_.Check1AndReset(
        extensions::NOTIFICATION_EXTENSION_REMOVED));
  }

  // Helper to remove a blocked extension and verify the result.
  void RemoveBlockedExtension() {
    // Calling RemoveExtension removes the extension.
    // TODO(michaelpg): Blocked extensions shouldn't need to be
    // "deactivated". See crbug.com/708230.
    EXPECT_CALL(delegate_, PostDeactivateExtension(extension_));
    registrar_->RemoveExtension(extension_->id(),
                                UnloadedExtensionReason::UNINSTALL);

    // RemoveExtension does not un-block the extension.
    ExpectInSet(ExtensionRegistry::BLOCKED);

    // Removing a blocked extension should trigger a notification.
    EXPECT_TRUE(notification_tracker_.Check1AndReset(
        extensions::NOTIFICATION_EXTENSION_REMOVED));
  }

  void EnableExtension() {
    ExtensionRegistry* extension_registry =
        ExtensionRegistry::Get(browser_context());

    EXPECT_CALL(delegate_, PostActivateExtension(extension_, false));
    registrar_->EnableExtension(extension_->id());
    ExpectInSet(ExtensionRegistry::ENABLED);
    EXPECT_FALSE(IsExtensionReady());

    TestExtensionRegistryObserver(extension_registry).WaitForExtensionReady();
    ExpectInSet(ExtensionRegistry::ENABLED);
    EXPECT_TRUE(IsExtensionReady());
  }

  void DisableExtension() {
    EXPECT_CALL(delegate_, PostDeactivateExtension(extension_));
    registrar_->DisableExtension(extension_->id(),
                                 disable_reason::DISABLE_USER_ACTION);
    ExpectInSet(ExtensionRegistry::DISABLED);
    EXPECT_FALSE(IsExtensionReady());
  }

  // Verifies that the extension is in the given set in the ExtensionRegistry
  // and not in other sets.
  void ExpectInSet(ExtensionRegistry::IncludeFlag set_id) {
    ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context());

    EXPECT_EQ(set_id == ExtensionRegistry::ENABLED,
              registry->enabled_extensions().Contains(extension_->id()));

    EXPECT_EQ(set_id == ExtensionRegistry::DISABLED,
              registry->disabled_extensions().Contains(extension_->id()));

    EXPECT_EQ(set_id == ExtensionRegistry::TERMINATED,
              registry->terminated_extensions().Contains(extension_->id()));

    EXPECT_EQ(set_id == ExtensionRegistry::BLACKLISTED,
              registry->blacklisted_extensions().Contains(extension_->id()));

    EXPECT_EQ(set_id == ExtensionRegistry::BLOCKED,
              registry->blocked_extensions().Contains(extension_->id()));
  }

  bool IsExtensionReady() {
    return ExtensionRegistry::Get(browser_context())
        ->ready_extensions()
        .Contains(extension_->id());
  }

  ExtensionRegistrar* registrar() { return &registrar_.value(); }
  TestExtensionRegistrarDelegate* delegate() { return &delegate_; }

  scoped_refptr<const Extension> extension() const { return extension_; }

 private:
  MockExtensionSystemFactory<TestExtensionSystem> factory_;
  testing::StrictMock<TestExtensionRegistrarDelegate> delegate_;
  scoped_refptr<const Extension> extension_;

  content::TestNotificationTracker notification_tracker_;

  // Initialized in SetUp().
  base::Optional<ExtensionRegistrar> registrar_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRegistrarTest);
};

// Adds and removes an extension.
TEST_F(ExtensionRegistrarTest, AddAndRemove) {
  AddEnabledExtension();
  RemoveEnabledExtension();
}

// Disables an extension before removing it.
TEST_F(ExtensionRegistrarTest, DisableAndRemove) {
  AddEnabledExtension();

  // Disable the extension before removing it.
  DisableExtension();
  RemoveDisabledExtension();
}

// Adds, disables and re-enables an extension.
TEST_F(ExtensionRegistrarTest, DisableAndEnable) {
  AddEnabledExtension();

  // Disable then enable the extension.
  DisableExtension();
  EnableExtension();

  RemoveEnabledExtension();
}

// Adds a disabled extension.
TEST_F(ExtensionRegistrarTest, AddDisabled) {
  // An extension can be added as disabled, then removed.
  AddDisabledExtension();
  RemoveDisabledExtension();

  // An extension can be added as disabled, then enabled.
  AddDisabledExtension();
  EnableExtension();
  RemoveEnabledExtension();
}

// Adds a force-enabled extension.
TEST_F(ExtensionRegistrarTest, AddForceEnabled) {
  // Prevent the extension from being disabled.
  EXPECT_CALL(*delegate(), CanDisableExtension(extension().get()))
      .WillRepeatedly(Return(false));
  AddEnabledExtension();

  // Extension cannot be disabled.
  registrar()->DisableExtension(extension()->id(),
                                disable_reason::DISABLE_USER_ACTION);
  ExpectInSet(ExtensionRegistry::ENABLED);
}

// Adds a force-disabled extension.
TEST_F(ExtensionRegistrarTest, AddForceDisabled) {
  // Prevent the extension from being enabled.
  EXPECT_CALL(*delegate(), CanEnableExtension(extension().get()))
      .WillRepeatedly(Return(false));
  AddDisabledExtension();

  // Extension cannot be enabled.
  registrar()->EnableExtension(extension()->id());
  ExpectInSet(ExtensionRegistry::DISABLED);
}

// Adds a blacklisted extension.
TEST_F(ExtensionRegistrarTest, AddBlacklisted) {
  AddBlacklistedExtension();
  RemoveBlacklistedExtension();
}

// Adds a blocked extension.
TEST_F(ExtensionRegistrarTest, AddBlocked) {
  // Block extensions.
  EXPECT_CALL(*delegate(), ShouldBlockExtension(extension().get()))
      .WillRepeatedly(Return(true));

  // A blocked extension can be added.
  AddBlockedExtension();

  // Extension cannot be enabled.
  registrar()->EnableExtension(extension()->id());
  ExpectInSet(ExtensionRegistry::BLOCKED);

  RemoveBlockedExtension();
}

}  // namespace extensions
