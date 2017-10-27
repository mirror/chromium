// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/lock_screen_action.h"

#include <memory>
#include <vector>

#include "ash/lock_screen_action/lock_screen_action_observer.h"
#include "ash/lock_screen_action/test_lock_screen_action_client.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"
#include "base/run_loop.h"

using ash::mojom::LockScreenActionState;

namespace ash {

namespace {

class ScopedTestStateObserver : public LockScreenActionObserver {
 public:
  explicit ScopedTestStateObserver(LockScreenAction* lock_screen_action)
      : lock_screen_action_(lock_screen_action) {
    lock_screen_action_->AddObserver(this);
  }

  ~ScopedTestStateObserver() override {
    lock_screen_action_->RemoveObserver(this);
  }

  // LockScreenActionObserver:
  void OnNoteStateChanged(LockScreenActionState state) override {
    observed_states_.push_back(state);
  }

  const std::vector<LockScreenActionState>& observed_states() const {
    return observed_states_;
  }

  void ClearObservedStates() { observed_states_.clear(); }

 private:
  LockScreenAction* lock_screen_action_;

  std::vector<LockScreenActionState> observed_states_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTestStateObserver);
};

using LockScreenActionTest = AshTestBase;

}  // namespace

TEST_F(LockScreenActionTest, NoLockScreenActionClient) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();
  ScopedTestStateObserver observer(lock_screen_action);

  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            lock_screen_action->GetNoteState());

  lock_screen_action->UpdateNoteState(LockScreenActionState::kAvailable);

  // The effective state should be |kNotAvailable| as long as an action handler
  // is not set.
  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            lock_screen_action->GetNoteState());
  EXPECT_EQ(0u, observer.observed_states().size());

  std::unique_ptr<TestLockScreenActionClient> action_client =
      std::make_unique<TestLockScreenActionClient>();
  lock_screen_action->SetClient(action_client->CreateInterfacePtrAndBind(),
                                LockScreenActionState::kLaunching);

  EXPECT_EQ(LockScreenActionState::kLaunching,
            lock_screen_action->GetNoteState());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kLaunching, observer.observed_states()[0]);
  observer.ClearObservedStates();

  action_client.reset();
  lock_screen_action->FlushMojoForTesting();

  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            lock_screen_action->GetNoteState());
  EXPECT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            observer.observed_states()[0]);
}

TEST_F(LockScreenActionTest, SettingInitialState) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();

  ScopedTestStateObserver observer(lock_screen_action);
  TestLockScreenActionClient action_client;
  lock_screen_action->SetClient(action_client.CreateInterfacePtrAndBind(),
                                LockScreenActionState::kAvailable);

  EXPECT_EQ(LockScreenActionState::kAvailable,
            lock_screen_action->GetNoteState());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kAvailable, observer.observed_states()[0]);
}

TEST_F(LockScreenActionTest, StateChangeNotificationOnConnectionLoss) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();

  ScopedTestStateObserver observer(lock_screen_action);
  std::unique_ptr<TestLockScreenActionClient> action_client(
      new TestLockScreenActionClient());
  lock_screen_action->SetClient(action_client->CreateInterfacePtrAndBind(),
                                LockScreenActionState::kAvailable);

  EXPECT_EQ(LockScreenActionState::kAvailable,
            lock_screen_action->GetNoteState());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kAvailable, observer.observed_states()[0]);
  observer.ClearObservedStates();

  action_client.reset();
  lock_screen_action->FlushMojoForTesting();

  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            lock_screen_action->GetNoteState());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            observer.observed_states()[0]);
}

TEST_F(LockScreenActionTest, NormalStateProgression) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();

  ScopedTestStateObserver observer(lock_screen_action);
  TestLockScreenActionClient action_client;
  lock_screen_action->SetClient(action_client.CreateInterfacePtrAndBind(),
                                LockScreenActionState::kNotAvailable);

  lock_screen_action->UpdateNoteState(LockScreenActionState::kAvailable);
  EXPECT_EQ(LockScreenActionState::kAvailable,
            lock_screen_action->GetNoteState());
  EXPECT_FALSE(lock_screen_action->IsNoteActive());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kAvailable, observer.observed_states()[0]);
  observer.ClearObservedStates();

  lock_screen_action->UpdateNoteState(LockScreenActionState::kLaunching);
  EXPECT_EQ(LockScreenActionState::kLaunching,
            lock_screen_action->GetNoteState());
  EXPECT_FALSE(lock_screen_action->IsNoteActive());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kLaunching, observer.observed_states()[0]);
  observer.ClearObservedStates();

  lock_screen_action->UpdateNoteState(LockScreenActionState::kActive);
  EXPECT_EQ(LockScreenActionState::kActive, lock_screen_action->GetNoteState());
  EXPECT_TRUE(lock_screen_action->IsNoteActive());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kActive, observer.observed_states()[0]);
  observer.ClearObservedStates();

  lock_screen_action->UpdateNoteState(LockScreenActionState::kNotAvailable);
  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            lock_screen_action->GetNoteState());
  EXPECT_FALSE(lock_screen_action->IsNoteActive());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kNotAvailable,
            observer.observed_states()[0]);
}

TEST_F(LockScreenActionTest, ObserversNotNotifiedOnDuplicateState) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();

  ScopedTestStateObserver observer(lock_screen_action);
  TestLockScreenActionClient action_client;
  lock_screen_action->SetClient(action_client.CreateInterfacePtrAndBind(),
                                LockScreenActionState::kNotAvailable);

  lock_screen_action->UpdateNoteState(LockScreenActionState::kAvailable);
  EXPECT_EQ(LockScreenActionState::kAvailable,
            lock_screen_action->GetNoteState());
  ASSERT_EQ(1u, observer.observed_states().size());
  EXPECT_EQ(LockScreenActionState::kAvailable, observer.observed_states()[0]);
  observer.ClearObservedStates();

  lock_screen_action->UpdateNoteState(LockScreenActionState::kAvailable);
  EXPECT_EQ(LockScreenActionState::kAvailable,
            lock_screen_action->GetNoteState());
  ASSERT_EQ(0u, observer.observed_states().size());
}

TEST_F(LockScreenActionTest, RequestAction) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();

  TestLockScreenActionClient action_client;
  lock_screen_action->SetClient(action_client.CreateInterfacePtrAndBind(),
                                LockScreenActionState::kNotAvailable);

  EXPECT_TRUE(action_client.note_origins().empty());
  lock_screen_action->RequestNewNote(
      mojom::LockScreenNoteOrigin::kLockScreenButtonTap);
  lock_screen_action->FlushMojoForTesting();
  EXPECT_TRUE(action_client.note_origins().empty());

  lock_screen_action->UpdateNoteState(LockScreenActionState::kAvailable);
  lock_screen_action->RequestNewNote(
      mojom::LockScreenNoteOrigin::kLockScreenButtonTap);
  lock_screen_action->FlushMojoForTesting();
  EXPECT_EQ(std::vector<mojom::LockScreenNoteOrigin>(
                {mojom::LockScreenNoteOrigin::kLockScreenButtonTap}),
            action_client.note_origins());
}

// Tests that there is no crash if handler is not set.
TEST_F(LockScreenActionTest, RequestActionWithNoHandler) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();
  lock_screen_action->RequestNewNote(
      mojom::LockScreenNoteOrigin::kLockScreenButtonTap);
  lock_screen_action->FlushMojoForTesting();
}

TEST_F(LockScreenActionTest, CloseLockScreenNote) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();

  TestLockScreenActionClient action_client;
  lock_screen_action->SetClient(action_client.CreateInterfacePtrAndBind(),
                                LockScreenActionState::kNotAvailable);

  lock_screen_action->UpdateNoteState(LockScreenActionState::kActive);
  EXPECT_TRUE(action_client.close_note_reasons().empty());
  lock_screen_action->CloseNote(
      mojom::CloseLockScreenNoteReason::kUnlockButtonPressed);
  lock_screen_action->FlushMojoForTesting();
  EXPECT_EQ(std::vector<mojom::CloseLockScreenNoteReason>(
                {mojom::CloseLockScreenNoteReason::kUnlockButtonPressed}),
            action_client.close_note_reasons());
}

// Tests that there is no crash if handler is not set.
TEST_F(LockScreenActionTest, CloseLockScreenNoteWithNoHandler) {
  LockScreenAction* lock_screen_action = Shell::Get()->lock_screen_action();
  lock_screen_action->CloseNote(
      mojom::CloseLockScreenNoteReason::kUnlockButtonPressed);
  lock_screen_action->FlushMojoForTesting();
}

}  // namespace ash
