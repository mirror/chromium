// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/lock_screen_action.h"

#include <utility>

#include "ash/lock_screen_action/lock_screen_action_observer.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"

namespace ash {

LockScreenAction::LockScreenAction() = default;

LockScreenAction::~LockScreenAction() = default;

void LockScreenAction::AddObserver(LockScreenActionObserver* observer) {
  observers_.AddObserver(observer);
}

void LockScreenAction::RemoveObserver(LockScreenActionObserver* observer) {
  observers_.RemoveObserver(observer);
}

void LockScreenAction::BindRequest(mojom::LockScreenActionRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

mojom::LockScreenActionState LockScreenAction::GetNoteState() const {
  if (!lock_screen_action_client_)
    return mojom::LockScreenActionState::kNotAvailable;
  return note_state_;
}

bool LockScreenAction::IsNoteActive() const {
  return GetNoteState() == mojom::LockScreenActionState::kActive;
}

void LockScreenAction::SetClient(
    mojom::LockScreenActionClientPtr lock_screen_action_client,
    mojom::LockScreenActionState note_state) {
  mojom::LockScreenActionState old_note_state = GetNoteState();

  lock_screen_action_client_ = std::move(lock_screen_action_client);

  if (lock_screen_action_client_) {
    // Makes sure the state is updated in case the connection is lost.
    lock_screen_action_client_.set_connection_error_handler(
        base::Bind(&LockScreenAction::SetClient, base::Unretained(this),
                   nullptr, mojom::LockScreenActionState::kNotAvailable));
    note_state_ = note_state;
  }

  // Setting action handler value can change effective state - notify observers
  // if that was the case.
  if (GetNoteState() != old_note_state)
    NotifyNoteStateChanged();
}

void LockScreenAction::UpdateNoteState(mojom::LockScreenActionState state) {
  if (state == note_state_)
    return;

  note_state_ = state;

  // If the client is not set, the effective state has not changed, so no need
  // to notify observers of a state change.
  if (lock_screen_action_client_)
    NotifyNoteStateChanged();
}

void LockScreenAction::RequestNewNote(mojom::LockScreenNoteOrigin origin) {
  if (GetNoteState() != mojom::LockScreenActionState::kAvailable)
    return;

  // An action state can be kAvailable only if |lock_screen_action_client_| is
  // set.
  DCHECK(lock_screen_action_client_);
  lock_screen_action_client_->RequestNewNote(origin);
}

void LockScreenAction::CloseNote(mojom::CloseLockScreenNoteReason reason) {
  if (lock_screen_action_client_)
    lock_screen_action_client_->CloseNote(reason);
}

void LockScreenAction::FlushMojoForTesting() {
  if (lock_screen_action_client_)
    lock_screen_action_client_.FlushForTesting();
}

void LockScreenAction::NotifyNoteStateChanged() {
  for (auto& observer : observers_)
    observer.OnNoteStateChanged(GetNoteState());
}

}  // namespace ash
