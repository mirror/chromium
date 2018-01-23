// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_VIEW_H_
#define UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_VIEW_H_

#include <memory>

#include "ui/chromeos/ksv/keyboard_shortcut_item.h"
#include "ui/chromeos/ksv/keyboard_shortcut_viewer_model_observer.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class Widget;
}  // namespace views

namespace keyboard_shortcut_viewer {

class KeyboardShortcutViewerModel;

class KeyboardShortcutView : public views::WidgetDelegateView,
                             public KeyboardShortcutViewerModelObserver {
 public:
  KeyboardShortcutView();
  ~KeyboardShortcutView() override;

  // Shows the Keyboard Shortcut Viewer window, or re-activates an existing one.
  static void Show();

 private:
  // KeyboardShortcutViewerModelObserver:
  void OnKeyboardShortcutViewerModelChanged() override;

  // views::WidgetDelegate:
  bool CanMaximize() const override;
  bool CanMinimize() const override;
  bool CanResize() const override;
  // TODO(wutao): need to customize the frame view header based on UX specs.
  views::ClientView* CreateClientView(views::Widget* widget) override;

  std::unique_ptr<KeyboardShortcutViewerModel> model_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutView);
};

}  // namespace keyboard_shortcut_viewer

#endif  // UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_VIEW_H_
