// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/debug/debug_context_menu_controller.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/debug_utils.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

struct DebugMenuEntry {
 public:
  DebugMenuEntry(const std::string& text,
                 base::Closure callback)
      : text(text), callback(callback) {}

  std::string text;
  base::Closure callback;
};

class DebugMenuModel : public ui::MenuModel {
 public:
  DebugMenuModel() {}

  bool HasIcons() const override { return false; }
  int GetItemCount() const override { return entries_.size(); }
  ItemType GetTypeAt(int index) const override { return TYPE_COMMAND; }
  ui::MenuSeparatorType GetSeparatorTypeAt(int index) const override {
      return ui::NORMAL_SEPARATOR;
  }
  int GetCommandIdAt(int index) const override { return index; }
  base::string16 GetLabelAt(int index) const override {
    return base::UTF8ToUTF16(entries_[index].text);
  }
  base::string16 GetSublabelAt(int index) const override {
    return base::string16();
  }
  base::string16 GetMinorTextAt(int index) const override {
    return base::string16();
  }
  bool IsItemDynamicAt(int index) const override { return false; }
  const gfx::FontList* GetLabelFontListAt(int index) const override {
    return nullptr;
  }
  bool GetAcceleratorAt(int index, ui::Accelerator* accelerator) const override {
    return false;
  }
  bool IsItemCheckedAt(int index) const override { return false; }
  int GetGroupIdAt(int index) const override { return 0; }
  bool GetIconAt(int index, gfx::Image* icon) override { return false; }
  ui::ButtonMenuItemModel* GetButtonMenuItemAt(int index) const override {
    return nullptr;
  }
  bool IsEnabledAt(int index) const override { return true; }
  bool IsVisibleAt(int index) const override { return true; }
  MenuModel* GetSubmenuModelAt(int index) const override { return nullptr; }
  void HighlightChangedTo(int index) override {}
  void ActivatedAt(int index) override {
    entries_[index].callback.Run();
  }

  void SetMenuModelDelegate(ui::MenuModelDelegate* delegate) override {}
  ui::MenuModelDelegate* GetMenuModelDelegate() const override {
    return nullptr;
  }

  void AddEntry(const std::string& text, base::Closure callback) {
    entries_.push_back(DebugMenuEntry(text, callback));
  }

 private:
  std::vector<DebugMenuEntry> entries_;
};

// Marks |view| in |color|, meaning that |view| gets a solid border of |color|
// and gets a light background shade of |color|.
void Mark(views::View* view, SkColor color) {
  view->SetBorder(views::CreateSolidBorder(1, color));
  view->SetBackground(views::CreateSolidBackground(
      SkColorSetA(color, 0x20)));
  view->SchedulePaint();
}

std::unique_ptr<DebugMenuModel> MakeDebugMenu(
                    DebugContextMenuController* controller,
                    View* view) {
  auto model = std::make_unique<DebugMenuModel>();

  model->AddEntry("Mark Black",
      base::Bind(&Mark, base::Unretained(view), SK_ColorBLACK));
  model->AddEntry("Mark Blue",
      base::Bind(&Mark, base::Unretained(view), SK_ColorBLUE));
  model->AddEntry("Mark Red",
      base::Bind(&Mark, base::Unretained(view), SK_ColorRED));
  model->AddEntry("Mark Green",
      base::Bind(&Mark, base::Unretained(view), SK_ColorGREEN));

  model->AddEntry("Print Children",
      base::Bind(&PrintViewHierarchy, base::Unretained(view)));
  model->AddEntry("Print Root Children",
      base::Bind(&PrintViewHierarchy, base::Unretained(
          view->GetWidget()->GetRootView())));

  return model;
}

}  // namespace

DebugContextMenuController::DebugContextMenuController() {}
DebugContextMenuController::~DebugContextMenuController() {}

void DebugContextMenuController::ShowDebugContextMenuForView(
    views::View* view, const gfx::Point& anchor) {
  menu_model_ = MakeDebugMenu(this, view);
  menu_runner_ = std::make_unique<views::MenuRunner>(menu_model_.get(), 0);
  menu_runner_->RunMenuAt(
      view->GetWidget(), nullptr, gfx::Rect(anchor, gfx::Size()),
      views::MENU_ANCHOR_TOPLEFT, ui::MENU_SOURCE_MOUSE);
}

}  // namespace views
