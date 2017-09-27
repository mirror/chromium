// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_NOTE_ACTION_BUBBLE_VIEW_H_
#define ASH_LOGIN_UI_NOTE_ACTION_BUBBLE_VIEW_H_

#include "ash/login/ui/non_accessible_view.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/tray_action/tray_action_observer.h"
#include "base/macros.h"
#include "base/scoped_observer.h"

namespace ash {

class TrayAction;

// View for lock sreen UI element for launching note taking action handler app.
// The element is a image button with semi-transparent buble background, which
// is expanded upon hovering/focusing the element.
// The bubble is quarter of a circle with the center in top right corner of the
// view (in LTR layout).
// The button is only visible if the lock screen note taking action is available
// (the view observes the action availability and updates itself accordingly).
class NoteActionBubbleView : public NonAccessibleView,
                             public TrayActionObserver {
 public:
  NoteActionBubbleView();
  ~NoteActionBubbleView() override;

  // TrayActionObserver:
  void OnLockScreenNoteStateChanged(mojom::TrayActionState state) override;

 private:
  class BackgroundView;
  class ActionButton;

  // Updates the bubble visibility depending on the note taking action state.
  void UpdateVisibility(mojom::TrayActionState action_state);

  ScopedObserver<TrayAction, TrayActionObserver> tray_action_observer_;

  // The background bubble view.
  BackgroundView* background_ = nullptr;

  // The actionable image button view.
  ActionButton* action_button_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(NoteActionBubbleView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_NOTE_ACTION_BUBBLE_VIEW_H_
