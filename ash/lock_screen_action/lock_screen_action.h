// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_H_
#define ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/lock_screen_action.mojom.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

class LockScreenActionObserver;

// Controller that ash can use to request a predefined set of actions to be
// performed by clients.
// The controller provides an interface to:
//  * Send a request to the client to handle an action.
//  * Observe the state of support for an action as reported by the current ash
//    client.
// Currently, only single action is supported - creating new note on the lock
// screen - Chrome handles this action by launching an app (if any) that is
// registered as a lock screen enabled action handler for the new note action.
class ASH_EXPORT LockScreenAction : public mojom::LockScreenAction {
 public:
  LockScreenAction();
  ~LockScreenAction() override;

  void AddObserver(LockScreenActionObserver* observer);
  void RemoveObserver(LockScreenActionObserver* observer);

  void BindRequest(mojom::LockScreenActionRequest request);

  // Gets last known handler state for the lock screen note action.
  // It will return kNotAvailable if an action handler has not been set using
  // |SetClient|.
  mojom::LockScreenActionState GetNoteState() const;

  // Helper method for determining if lock screen not action is in active state.
  bool IsNoteActive() const;

  // If the client is set, sends it a request to handle the lock screen note
  // action.
  void RequestNewNote(mojom::LockScreenNoteOrigin origin);

  // If the client is set, sends a request to close the lock screen note.
  void CloseNote(mojom::CloseLockScreenNoteReason reason);

  // mojom::LockScreenAction:
  void SetClient(mojom::LockScreenActionClientPtr action_handler,
                 mojom::LockScreenActionState note_state) override;
  void UpdateNoteState(mojom::LockScreenActionState state) override;

  void FlushMojoForTesting();

 private:
  // Notifies the observers that state for the lock screen note action has been
  // updated.
  void NotifyNoteStateChanged();

  // Last known state for lock screen note action.
  mojom::LockScreenActionState note_state_ =
      mojom::LockScreenActionState::kNotAvailable;

  base::ObserverList<LockScreenActionObserver> observers_;

  mojo::BindingSet<mojom::LockScreenAction> bindings_;

  mojom::LockScreenActionClientPtr lock_screen_action_client_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenAction);
};

}  // namespace ash

#endif  // ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_H_
