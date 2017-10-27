// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOCK_SCREEN_ACTION_TEST_LOCK_SCREEN_ACTION_CLIENT_H_
#define ASH_LOCK_SCREEN_ACTION_TEST_LOCK_SCREEN_ACTION_CLIENT_H_

#include <vector>

#include "ash/public/interfaces/lock_screen_action.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

class TestLockScreenActionClient : public mojom::LockScreenActionClient {
 public:
  TestLockScreenActionClient();

  ~TestLockScreenActionClient() override;

  mojom::LockScreenActionClientPtr CreateInterfacePtrAndBind();

  void ClearRecordedRequests();

  const std::vector<mojom::LockScreenNoteOrigin>& note_origins() const {
    return note_origins_;
  }

  const std::vector<mojom::CloseLockScreenNoteReason>& close_note_reasons()
      const {
    return close_note_reasons_;
  }

  // mojom::LockScreenActionClient:
  void RequestNewNote(mojom::LockScreenNoteOrigin origin) override;
  void CloseNote(mojom::CloseLockScreenNoteReason reason) override;

 private:
  mojo::Binding<mojom::LockScreenActionClient> binding_;

  std::vector<mojom::LockScreenNoteOrigin> note_origins_;
  std::vector<mojom::CloseLockScreenNoteReason> close_note_reasons_;

  DISALLOW_COPY_AND_ASSIGN(TestLockScreenActionClient);
};

}  // namespace ash

#endif  // ASH_LOCK_SCREEN_ACTION_TEST_LOCK_SCREEN_ACTION_CLIENT_H_
