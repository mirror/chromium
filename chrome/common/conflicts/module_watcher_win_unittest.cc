// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/conflicts/module_watcher_win.h"

#include <memory>

#include "base/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// This module should not be a static dependency of the unit-test
// executable, but should be a build-system dependency or a module that is
// present on any Windows machine.
constexpr wchar_t kTestDllName[] = L"conflicts_dll.dll";

}  // namespace

class ModuleWatcherTest : public testing::Test {
 protected:
  ModuleWatcherTest()
      : module_(nullptr),
        module_event_count_(0),
        module_already_loaded_event_count_(0),
        module_loaded_event_count_(0),
        module_unloaded_event_count_(0),
        test_dll_event_count_(0) {}

  void OnModuleEvent(const ModuleWatcher::ModuleEvent& event) {
    ++module_event_count_;
    switch (event.event_type) {
      case mojom::ModuleEventType::MODULE_ALREADY_LOADED:
        ++module_already_loaded_event_count_;
        break;
      case mojom::ModuleEventType::MODULE_LOADED:
        ++module_loaded_event_count_;
        break;
      case mojom::ModuleEventType::MODULE_UNLOADED:
        ++module_unloaded_event_count_;
        break;
    }

    if (event.module_path.BaseName().value() == kTestDllName)
      ++test_dll_event_count_;
  }

  void TearDown() override { UnloadModule(); }

  void LoadModule() {
    if (module_)
      return;
    // The module should not already be loaded.
    ASSERT_FALSE(::GetModuleHandle(kTestDllName));
    // It should load successfully.
    module_ = ::LoadLibrary(kTestDllName);
    ASSERT_TRUE(module_);
  }

  void UnloadModule() {
    if (!module_)
      return;
    ::FreeLibrary(module_);
    module_ = nullptr;
  }

  std::unique_ptr<ModuleWatcher> Create() {
    return ModuleWatcher::Create(
        base::Bind(&ModuleWatcherTest::OnModuleEvent, base::Unretained(this)));
  }

  // Holds a handle to a loaded module.
  HMODULE module_;
  // Total number of module events seen.
  int module_event_count_;
  // Total number of MODULE_ALREADY_LOADED events seen.
  int module_already_loaded_event_count_;
  // Total number of MODULE_LOADED events seen.
  int module_loaded_event_count_;
  // Total number of MODULE_UNLOADED events seen.
  int module_unloaded_event_count_;
  // Total number of module events related to kTestDllName.
  int test_dll_event_count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ModuleWatcherTest);
};

TEST_F(ModuleWatcherTest, SingleModuleWatcherOnly) {
  std::unique_ptr<ModuleWatcher> mw1(Create());
  EXPECT_TRUE(mw1.get());

  std::unique_ptr<ModuleWatcher> mw2(Create());
  EXPECT_FALSE(mw2.get());
}

TEST_F(ModuleWatcherTest, ModuleEvents) {
  std::unique_ptr<ModuleWatcher> mw(Create());
  // Initialize the module watcher. This should immediately enumerate all
  // already loaded modules.
  mw->Initialize();
  EXPECT_LT(0, module_event_count_);
  EXPECT_LT(0, module_already_loaded_event_count_);
  EXPECT_EQ(0, module_loaded_event_count_);
  EXPECT_EQ(0, module_unloaded_event_count_);

  // Dynamically load a module and ensure a notification is received for it.
  int previous_module_loaded_event_count = module_loaded_event_count_;
  LoadModule();
  EXPECT_LT(previous_module_loaded_event_count, module_loaded_event_count_);

  // Unload the module and ensure another notification is received.
  int previous_module_unloaded_event_count = module_unloaded_event_count_;
  UnloadModule();
  EXPECT_LT(previous_module_unloaded_event_count, module_unloaded_event_count_);

  // Dynamically load a module and ensure a notification is received for it.
  previous_module_loaded_event_count = module_loaded_event_count_;
  LoadModule();
  EXPECT_LT(previous_module_loaded_event_count, module_loaded_event_count_);

  // Destroy the module watcher.
  mw.reset();

  // Unload the module and ensure no notification is received this time.
  previous_module_unloaded_event_count = module_unloaded_event_count_;
  UnloadModule();
  EXPECT_EQ(previous_module_unloaded_event_count, module_unloaded_event_count_);
}

// Simulate a race where dll notifications happens before enumerating already
// loaded dlls.
TEST_F(ModuleWatcherTest, ReconcilePendingEvents) {
  std::unique_ptr<ModuleWatcher> mw(Create());

  // Do the first part of the initialization.
  mw->RegisterDllNotificationCallback();

  EXPECT_EQ(0, module_event_count_);
  EXPECT_EQ(0, module_already_loaded_event_count_);
  EXPECT_EQ(0, module_loaded_event_count_);
  EXPECT_EQ(0, module_unloaded_event_count_);
  EXPECT_EQ(0, test_dll_event_count_);

  // Dynamically load a module and ensure no notification is received for it.
  int previous_module_loaded_event_count = module_loaded_event_count_;
  LoadModule();
  EXPECT_EQ(previous_module_loaded_event_count, module_loaded_event_count_);

  // Unload the module and ensure no notification is received for it.
  int previous_module_unloaded_event_count = module_unloaded_event_count_;
  UnloadModule();
  EXPECT_EQ(previous_module_unloaded_event_count, module_unloaded_event_count_);

  // Finish the initialization.
  mw->EnumerateAlreadyLoadedModules();

  // Even though the test dll was not loaded during enumeration, the
  // ModuleWatcher should have sent a MODULE_ALREADY_LOADED and a
  // MODULE_UNLOADED event for it.
  EXPECT_EQ(1, module_unloaded_event_count_);
  EXPECT_EQ(2, test_dll_event_count_);
}
