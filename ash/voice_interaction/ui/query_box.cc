// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/voice_interaction/ui/query_box.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shell.h"
#include "ash/voice_interaction/voice_interaction_controller.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/aura/client/window_types.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/hit_test.h"
#include "ui/events/keycodes/keyboard_codes_posix.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

constexpr int kQueryBoxHeight = 48;
constexpr int kQueryBoxWidth = 400;
QueryBox* g_query_box_view = nullptr;

}  // namespace

// static
void QueryBox::Show() {
  if (g_query_box_view) {
    g_query_box_view->GetWidget()->Activate();
  } else {
    g_query_box_view = new QueryBox();
    g_query_box_view->GetWidget()->Show();
  }
}

QueryBox::QueryBox() {
  auto* textfield = new views::Textfield();
  textfield->set_controller(this);
  textfield->SetBounds(0, 0, kQueryBoxWidth, kQueryBoxHeight);
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.delegate = this;
  auto* root_window = Shell::GetPrimaryRootWindow();
  params.parent =
      Shell::GetContainer(root_window, kShellWindowId_AlwaysOnTopContainer);
  Init(params);
  GetNativeWindow()->SetBounds(gfx::Rect(
      0, root_window->bounds().height() - kShelfSize - kQueryBoxHeight,
      kQueryBoxWidth, kQueryBoxHeight));
  views::Widget::GetContentsView()->AddChildView(textfield);
}

QueryBox::~QueryBox() = default;

bool QueryBox::ShouldShowCloseButton() const {
  return false;
}

bool QueryBox::ShouldShowWindowTitle() const {
  return false;
}

bool QueryBox::HandleKeyEvent(views::Textfield* sender,
                              const ui::KeyEvent& key_event) {
  if (key_event.key_code() == ui::VKEY_RETURN &&
      key_event.type() == ui::ET_KEY_PRESSED) {
    const base::string16 text = sender->text();
    sender->SetText(base::ASCIIToUTF16(""));
    LOG(ERROR) << "Got text input " << text;
    Shell::Get()->voice_interaction_controller()->SendTextQuery(
        base::UTF16ToUTF8(text));
    return true;
  }
  return false;
}

views::Widget* QueryBox::GetWidget() {
  return this;
}

const views::Widget* QueryBox::GetWidget() const {
  return this;
}

}  // namespace ash
