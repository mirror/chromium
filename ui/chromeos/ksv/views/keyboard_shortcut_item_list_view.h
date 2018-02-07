// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_ITEM_LIST_VIEW_H_
#define UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_ITEM_LIST_VIEW_H_

#include "ui/chromeos/ksv/views/keyboard_shortcut_item_view.h"
#include "ui/views/view.h"

namespace keyboard_shortcut_viewer {

class KeyboardShortcutItemView;

// Displays a list of KeyboardShortcutItemView. In search result page, all
// KeyboardShortcutItemView are grouped by ShortcutCategory and can be scrolled
// in a single page. A text label to indicate the ShortcutCategory will be
// diplayed at the beginning of the group of KeyboardShortcutItemView.
class KeyboardShortcutItemListView : public views::View {
 public:
  KeyboardShortcutItemListView();
  ~KeyboardShortcutItemListView() override = default;

  void AddItemView(KeyboardShortcutItemView* item_view);

  // In search result page, a text label is added at the beginning of the group
  // of KeyboardShortcutItemView to indicate the ShortcutCategory.
  void AddCategoryLabel(const base::string16& text);

  views::View* shortcut_item_views() { return shortcut_item_views_; }

 private:
  // The parent view of the list of KeyboardShortcutItemView.
  views::View* shortcut_item_views_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutItemListView);
};

}  // namespace keyboard_shortcut_viewer

#endif  // UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_ITEM_LIST_VIEW_H_
