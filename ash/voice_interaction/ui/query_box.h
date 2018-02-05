// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_VOICE_INTERACTION_UI_QUERY_BOX_H_
#define ASH_VOICE_INTERACTION_UI_QUERY_BOX_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class Textfield;
}  // namespace views

namespace ui {
class KeyEvent;
}  // namespace ui

namespace ash {

class ASH_EXPORT QueryBox : public views::WidgetDelegate,
                            public views::Widget,
                            public views::TextfieldController {
 public:
  static void Show();

  ~QueryBox() override;

  // views::WIdgetDelegateView overrides
  bool ShouldShowCloseButton() const override;
  bool ShouldShowWindowTitle() const override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

  // views::TextfieldController overrides.
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

 private:
  QueryBox();
};

}  // namespace ash

#endif  // ASH_VOICE_INTERACTION_UI_QUERY_BOX_H_
