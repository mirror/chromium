// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/views/keyboard_shortcut_view.h"

#include "ui/chromeos/ksv/keyboard_shortcut_viewer_model.h"
#include "ui/views/background.h"
#include "ui/views/widget/widget.h"

namespace keyboard_shortcut_viewer {

namespace {

KeyboardShortcutView* g_ksv_view = nullptr;

}  // namespace

// static
void KeyboardShortcutView::Show() {
  if (g_ksv_view) {
    // If there is a KeyboardShortcutView window open already, just activate it.
    g_ksv_view->GetWidget()->Activate();
    return;
  }

  // TODO(wutao): change the bounds to UX specs when they are available.
  g_ksv_view = new KeyboardShortcutView();
  views::Widget::CreateWindowWithParentAndBounds(g_ksv_view, nullptr,
                                                 gfx::Rect(0, 0, 660, 560));
  g_ksv_view->GetWidget()->Show();
}

KeyboardShortcutView::KeyboardShortcutView() {
  // Default background is transparent.
  SetBackground(views::CreateSolidBackground(SK_ColorWHITE));

  model_ = std::make_unique<KeyboardShortcutViewerModel>();
  model_->AddObserver(this);
  model_->MakeKeyboardShortcutItems();
}

KeyboardShortcutView::~KeyboardShortcutView() {
  model_->RemoveObserver(this);
  g_ksv_view = nullptr;
}

void KeyboardShortcutView::OnKeyboardShortcutViewerModelChanged() {
  // TODO(wutao): implement convertion from KeyboardShortcutItem to
  // KeyboardShortcutItemView, including generating strings and icons for VKEY.
  // Read model_->shortcut_items() to constuct the views.
  NOTIMPLEMENTED();
}

bool KeyboardShortcutView::CanMaximize() const {
  return false;
}

bool KeyboardShortcutView::CanMinimize() const {
  return true;
}

bool KeyboardShortcutView::CanResize() const {
  return false;
}

views::ClientView* KeyboardShortcutView::CreateClientView(
    views::Widget* widget) {
  return new views::ClientView(widget, this);
}

}  // namespace keyboard_shortcut_viewer
