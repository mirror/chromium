// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/views/keyboard_shortcut_view.h"

#include "ui/views/background.h"
#include "ui/views/widget/widget.h"

namespace keyboard_shortcut_viewer {

namespace {

KeyboardShortcutView* g_ksv_view = nullptr;

}  // namespace

KeyboardShortcutView::~KeyboardShortcutView() = default;

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

  InitViews();
}

void KeyboardShortcutView::InitViews() {
  // TODO(wutao): implement convertion from KeyboardShortcutItem to
  // KeyboardShortcutItemView, including generating strings and icons for VKEY.
  // Call GetKeyboardShortcutItemList() to constuct the item views.
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

void KeyboardShortcutView::WindowClosing() {
  // Now that the window is closed, we can allow a new one to be opened.
  // (WindowClosing comes in asynchronously from the call to Close() and we
  // may have already opened a new instance).
  if (g_ksv_view == this) {
    // We don't have to delete |g_ksv_view| as we don't own it. It's
    // owned by the Views hierarchy.
    g_ksv_view = nullptr;
  }
}

}  // namespace keyboard_shortcut_viewer
