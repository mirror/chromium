// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/views/keyboard_shortcut_item_list_view.h"

#include "ui/base/default_style.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/chromeos/ksv/views/keyboard_shortcut_item_view.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace keyboard_shortcut_viewer {

KeyboardShortcutItemListView::KeyboardShortcutItemListView()
    : shortcut_item_views_(new views::View) {
  auto layout = std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical);
  layout->set_main_axis_alignment(views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  shortcut_item_views_->SetLayoutManager(std::move(layout));

  views::ScrollView* const scroller = new views::ScrollView();
  scroller->set_draw_overflow_indicator(false);
  scroller->ClipHeightTo(0, 0);
  scroller->SetContents(shortcut_item_views_);
  AddChildView(scroller);
  SetBorder(views::CreateEmptyBorder(gfx::Insets(0, 32, 0, 32)));
  SetLayoutManager(std::make_unique<views::FillLayout>());
}

void KeyboardShortcutItemListView::AddItemView(
    KeyboardShortcutItemView* item_view) {
  shortcut_item_views_->AddChildView(item_view);
}

void KeyboardShortcutItemListView::AddCategoryLabel(
    const base::string16& text) {
  views::Label* category_label = new views::Label(text);
  category_label->SetHorizontalAlignment(gfx::ALIGN_TO_HEAD);
  category_label->SetBorder(
      views::CreateEmptyBorder(gfx::Insets(44, 0, 20, 0)));
  category_label->SetEnabledColor(SkColorSetARGBMacro(0xFF, 0x42, 0x85, 0xF4));
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  category_label->SetFontList(rb.GetFontListWithDelta(
      ui::kLabelFontSizeDelta, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));
  shortcut_item_views_->AddChildView(category_label);
}

}  // namespace keyboard_shortcut_viewer
