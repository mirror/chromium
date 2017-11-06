// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/find_bar_icon.h"

#include "components/toolbar/vector_icons.h"

FindBarIcon::FindBarIcon() : BubbleIconView(nullptr, 0) {}

FindBarIcon::~FindBarIcon() {}

bool FindBarIcon::IsActivated() {
  return GetInkDrop()->GetTargetInkDropState() ==
         views::InkDropState::ACTIVATED;
}

void FindBarIcon::SetActive(bool active, bool animate) {
  if (active) {
    if (animate) {
      AnimateInkDrop(views::InkDropState::ACTIVATED, nullptr);
    } else {
      GetInkDrop()->SnapToActivated();
    }
  } else {
    AnimateInkDrop(views::InkDropState::HIDDEN, nullptr);
  }
}

void FindBarIcon::OnExecuting(ExecuteSource execute_source) {}

views::BubbleDialogDelegateView* FindBarIcon::GetBubble() const {
  return nullptr;
}

const gfx::VectorIcon& FindBarIcon::GetVectorIcon() const {
  return toolbar::kFindInPageIcon;
}
