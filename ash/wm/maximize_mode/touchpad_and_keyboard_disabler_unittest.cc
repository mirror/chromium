// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/maximize_mode/touchpad_and_keyboard_disabler.h"

#include <memory>
#include <utility>

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_environment.h"
#include "ash/test/ash_test_helper.h"
#include "base/bind.h"

namespace ash {
namespace {

class TestDelegate : public TouchpadAndKeyboardDisabler::Delegate {
 public:
  TestDelegate(bool* destroyed,
               int* hide_cursor_call_count,
               int* enable_call_count)
      : destroyed_(destroyed),
        hide_cursor_call_count_(hide_cursor_call_count),
        enable_call_count_(enable_call_count) {}
  ~TestDelegate() override { *destroyed_ = true; }

  DisableResultClosure disable_callback() {
    return std::move(disable_callback_);
  }

  bool is_disable_callback_valid() const {
    return !disable_callback_.is_null();
  }

  void RunDisableCallback(bool result) {
    std::move(disable_callback_).Run(result);
  }

  // TouchpadAndKeyboardDisabler::Delegate:
  void Disable(DisableResultClosure callback) override {
    disable_callback_ = std::move(callback);
  }
  void HideCursor() override { (*hide_cursor_call_count_)++; }
  void Enable() override { (*enable_call_count_)++; }

 private:
  bool* destroyed_;
  DisableResultClosure disable_callback_;
  int* hide_cursor_call_count_;
  int* enable_call_count_;

  DISALLOW_COPY_AND_ASSIGN(TestDelegate);
};

struct TestHelper {
  TestHelper() {
    std::unique_ptr<TestDelegate> owned_test_delegate =
        base::MakeUnique<TestDelegate>(
            &delegate_destroyed, &hide_cursor_call_count, &enable_call_count);
    test_delegate = owned_test_delegate.get();
    disabler = new TouchpadAndKeyboardDisabler(std::move(owned_test_delegate));
  }

  int hide_cursor_call_count = 0;
  int enable_call_count = 0;
  bool delegate_destroyed = false;
  TouchpadAndKeyboardDisabler* disabler = nullptr;
  // Owned by TouchpadDisabler.
  TestDelegate* test_delegate = nullptr;
};

}  // namespace

using TouchpadDisablerTest = test::AshTestBase;

TEST_F(TouchpadDisablerTest, DisableCallbackSucceeds) {
  TestHelper helper;
  ASSERT_TRUE(helper.test_delegate->is_disable_callback_valid());
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.test_delegate->RunDisableCallback(true);
  EXPECT_EQ(1, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.disabler->Destroy();
  EXPECT_EQ(1, helper.hide_cursor_call_count);
  EXPECT_EQ(1, helper.enable_call_count);
  EXPECT_TRUE(helper.delegate_destroyed);
}

TEST_F(TouchpadDisablerTest, DisableCallbackFails) {
  TestHelper helper;
  ASSERT_TRUE(helper.test_delegate->is_disable_callback_valid());
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.test_delegate->RunDisableCallback(false);
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.disabler->Destroy();
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  EXPECT_TRUE(helper.delegate_destroyed);
}

TEST_F(TouchpadDisablerTest, DisableCallbackSucceedsAfterDestroy) {
  TestHelper helper;
  ASSERT_TRUE(helper.test_delegate->is_disable_callback_valid());
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.disabler->Destroy();
  ASSERT_FALSE(helper.delegate_destroyed);
  helper.test_delegate->RunDisableCallback(true);
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(1, helper.enable_call_count);
  EXPECT_TRUE(helper.delegate_destroyed);
}

TEST_F(TouchpadDisablerTest, DisableCallbackFailsAfterDestroy) {
  TestHelper helper;
  ASSERT_TRUE(helper.test_delegate->is_disable_callback_valid());
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.disabler->Destroy();
  ASSERT_FALSE(helper.delegate_destroyed);
  helper.test_delegate->RunDisableCallback(false);
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  EXPECT_TRUE(helper.delegate_destroyed);
}

TEST_F(TouchpadDisablerTest, CallbackSucceedsAfterDestroy) {
  TestHelper helper;
  ASSERT_TRUE(helper.test_delegate->is_disable_callback_valid());
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.disabler->Destroy();
  ASSERT_FALSE(helper.delegate_destroyed);
  helper.test_delegate->RunDisableCallback(true);
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(1, helper.enable_call_count);
  ASSERT_TRUE(helper.delegate_destroyed);
}

TEST_F(TouchpadDisablerTest, CallbackFailsAfterDestroy) {
  TestHelper helper;
  ASSERT_TRUE(helper.test_delegate->is_disable_callback_valid());
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.disabler->Destroy();
  ASSERT_FALSE(helper.delegate_destroyed);
  helper.test_delegate->RunDisableCallback(false);
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  ASSERT_TRUE(helper.delegate_destroyed);
}

using TouchpadDisablerTest2 = testing::Test;

TEST_F(TouchpadDisablerTest2, DisableCallbackSucceedsAfterShellDestroyed) {
  std::unique_ptr<test::AshTestEnvironment> ash_test_environment =
      test::AshTestEnvironment::Create();
  std::unique_ptr<test::AshTestHelper> ash_test_helper =
      base::MakeUnique<test::AshTestHelper>(ash_test_environment.get());
  const bool start_session = true;
  ash_test_helper->SetUp(start_session);

  TestHelper helper;
  ASSERT_TRUE(helper.test_delegate->is_disable_callback_valid());
  TestDelegate::DisableResultClosure disable_result_closure =
      helper.test_delegate->disable_callback();
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
  helper.disabler->Destroy();
  ash_test_helper->RunAllPendingInMessageLoop();
  ash_test_helper->TearDown();
  ash_test_helper.reset();
  ASSERT_FALSE(Shell::HasInstance());
  EXPECT_TRUE(helper.delegate_destroyed);
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);

  // Run the callback, just to be sure nothing bad happens.
  std::move(disable_result_closure).Run(true);
  EXPECT_TRUE(helper.delegate_destroyed);
  EXPECT_EQ(0, helper.hide_cursor_call_count);
  EXPECT_EQ(0, helper.enable_call_count);
}

}  // namespace ash
