// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/key_hook_state_keeper.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace keyboard_lock {

namespace {

static const ui::KeyboardCode KEY_1 = ui::VKEY_CANCEL;
static const ui::KeyboardCode KEY_2 = ui::VKEY_BACK;
static const ui::KeyboardCode KEY_3 = ui::VKEY_TAB;

class FakeKeyHook : public KeyHook {
 public:
  FakeKeyHook();
  ~FakeKeyHook() override = default;

  // KeyHook implementations.
  void RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                   base::Callback<void(bool)> on_result) override;
  void UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                     base::Callback<void(bool)> on_result) override;

  bool registered(ui::KeyboardCode code) const { return states_[code]; }
  int register_counter() const { return register_counter_; }
  int unregister_counter() const { return unregister_counter_; }

 private:
  std::vector<bool> states_;
  int register_counter_ = 0;
  int unregister_counter_ = 0;
};

class FakeKeyEventFilter : public KeyEventFilter {
 public:
  explicit FakeKeyEventFilter(const FakeKeyHook* const key_hook);
  ~FakeKeyEventFilter() override = default;

  // KeyEventFilter implementations.
  bool OnKeyDown(ui::KeyboardCode code, int flags) override;
  bool OnKeyUp(ui::KeyboardCode code, int flags) override;

 private:
  const FakeKeyHook* const key_hook_;
};

FakeKeyHook::FakeKeyHook() : states_(256) {}

void FakeKeyHook::RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                              base::Callback<void(bool)> on_result) {
  register_counter_++;
  for (const ui::KeyboardCode code : codes) {
    ASSERT_LT(code, 256);
    ASSERT_GE(code, 0);
    ASSERT_FALSE(states_[code]);
    states_[code] = true;
  }
  if (on_result) {
    on_result.Run(true);
  }
}

void FakeKeyHook::UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                                base::Callback<void(bool)> on_result) {
  unregister_counter_++;
  for (const ui::KeyboardCode code : codes) {
    ASSERT_LT(code, 256);
    ASSERT_GE(code, 0);
    ASSERT_TRUE(states_[code]);
    states_[code] = false;
  }
  if (on_result) {
    on_result.Run(true);
  }
}

FakeKeyEventFilter::FakeKeyEventFilter(const FakeKeyHook* const key_hook)
    : key_hook_(key_hook) {
  DCHECK(key_hook_);
}

bool FakeKeyEventFilter::OnKeyDown(ui::KeyboardCode code, int flags) {
  EXPECT_LT(code, 256);
  EXPECT_GE(code, 0);
  return key_hook_->registered(code);
}

bool FakeKeyEventFilter::OnKeyUp(ui::KeyboardCode code, int flags) {
  EXPECT_LT(code, 256);
  EXPECT_GE(code, 0);
  return key_hook_->registered(code);
}

}

class KeyHookStateKeeperTest : public testing::Test {
 public:
  KeyHookStateKeeperTest() {
    std::unique_ptr<FakeKeyHook> hook(new FakeKeyHook());
    key_hook_ = hook.get();
    state_keeper_.reset(new KeyHookStateKeeper(
        base::MakeUnique<FakeKeyEventFilter>(key_hook_),
        std::move(hook)));
    callback_ = base::Bind([](bool* target, bool result) {
      *target = result;
    },
    base::Unretained(&last_callback_result_));
  }

 protected:
  const FakeKeyHook* key_hook() const { return key_hook_; }
  KeyHookStateKeeper* state_keeper() const { return state_keeper_.get(); }
  base::Callback<void(bool)> callback() const { return callback_; }
  bool last_callback_result() const { return last_callback_result_; }

 private:
  const FakeKeyHook* key_hook_;
  std::unique_ptr<KeyHookStateKeeper> state_keeper_;
  base::Callback<void(bool)> callback_;
  bool last_callback_result_ = false;
};

TEST_F(KeyHookStateKeeperTest, RegisterBeforeActivate) {
  state_keeper()->RegisterKey({ KEY_1 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);
}

TEST_F(KeyHookStateKeeperTest, RegisterAfterActivate) {
  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->RegisterKey({ KEY_1 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);
}

TEST_F(KeyHookStateKeeperTest, ActivateInTheMiddle) {
  state_keeper()->RegisterKey({ KEY_1 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->RegisterKey({ KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_2));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 2);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);
}

TEST_F(KeyHookStateKeeperTest, ActivateTwiceWontTakeEffect) {
  state_keeper()->RegisterKey({ KEY_1 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);
}

TEST_F(KeyHookStateKeeperTest, DeactivateAfterActivate) {
  state_keeper()->RegisterKey({ KEY_1 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));

  state_keeper()->Deactivate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 1);
}

TEST_F(KeyHookStateKeeperTest, DeactivateTwiceWontTakeEffect) {
  state_keeper()->RegisterKey({ KEY_1 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 0);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);

  state_keeper()->Deactivate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 1);

  state_keeper()->Deactivate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 1);
}

TEST_F(KeyHookStateKeeperTest, UnregisterAfterActivate) {
  state_keeper()->RegisterKey({ KEY_1, KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 0);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_TRUE(key_hook()->registered(KEY_2));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);

  state_keeper()->UnregisterKey({ KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 1);
}

TEST_F(KeyHookStateKeeperTest, RegisteredKeysWontBeRegisteredAgain) {
  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());

  state_keeper()->RegisterKey({ KEY_1, KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(key_hook()->registered(KEY_2));

  state_keeper()->RegisterKey({ KEY_1, KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(key_hook()->registered(KEY_2));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);

  state_keeper()->UnregisterKey({ KEY_1, KEY_3 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(key_hook()->registered(KEY_3));
  ASSERT_TRUE(key_hook()->registered(KEY_2));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 1);

  state_keeper()->UnregisterKey({ KEY_1, KEY_3 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(key_hook()->registered(KEY_3));
  ASSERT_TRUE(key_hook()->registered(KEY_2));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 1);

  state_keeper()->UnregisterKey({ KEY_1, KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(key_hook()->registered(KEY_3));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 2);
}

TEST_F(KeyHookStateKeeperTest, MoreCombinations) {
  state_keeper()->RegisterKey({ KEY_1, KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 0);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_TRUE(key_hook()->registered(KEY_2));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 0);

  state_keeper()->UnregisterKey({ KEY_2 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 1);

  state_keeper()->Deactivate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 2);

  state_keeper()->RegisterKey({ KEY_3 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_3));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_3, 0));

  ASSERT_EQ(key_hook()->register_counter(), 1);
  ASSERT_EQ(key_hook()->unregister_counter(), 2);

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_TRUE(key_hook()->registered(KEY_1));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));
  ASSERT_TRUE(key_hook()->registered(KEY_3));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_3, 0));

  ASSERT_EQ(key_hook()->register_counter(), 2);
  ASSERT_EQ(key_hook()->unregister_counter(), 2);

  state_keeper()->Deactivate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_3));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_3, 0));

  ASSERT_EQ(key_hook()->register_counter(), 2);
  ASSERT_EQ(key_hook()->unregister_counter(), 3);

  state_keeper()->UnregisterKey({ KEY_1 }, callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_3));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_3, 0));

  ASSERT_EQ(key_hook()->register_counter(), 2);
  ASSERT_EQ(key_hook()->unregister_counter(), 3);

  state_keeper()->Activate(callback());
  ASSERT_TRUE(last_callback_result());
  ASSERT_FALSE(key_hook()->registered(KEY_1));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_1, 0));
  ASSERT_FALSE(key_hook()->registered(KEY_2));
  ASSERT_FALSE(state_keeper()->OnKeyDown(KEY_2, 0));
  ASSERT_TRUE(key_hook()->registered(KEY_3));
  ASSERT_TRUE(state_keeper()->OnKeyDown(KEY_3, 0));

  ASSERT_EQ(key_hook()->register_counter(), 3);
  ASSERT_EQ(key_hook()->unregister_counter(), 3);
}

}  // namespace keyboard_lock
