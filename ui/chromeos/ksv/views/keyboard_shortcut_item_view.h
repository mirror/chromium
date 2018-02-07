// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_ITEM_VIEW_H_
#define UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_ITEM_VIEW_H_

#include "ui/chromeos/ksv/keyboard_shortcut_item.h"
#include "ui/views/view.h"

namespace views {
class StyledLabel;
}  //  namespace views

namespace keyboard_shortcut_viewer {

struct KeyboardShortcutItem;
enum class ShortcutCategory;

// A view that displays a shortcut metadata.
class KeyboardShortcutItemView : public views::View {
 public:
  KeyboardShortcutItemView(const KeyboardShortcutItem& item,
                           ShortcutCategory category);
  ~KeyboardShortcutItemView() override = default;

  // views::View:
  int GetHeightForWidth(int w) const override;
  void Layout() override;

  views::StyledLabel* description_label_view() {
    return description_label_view_;
  }

  views::StyledLabel* shortcut_label_view() { return shortcut_label_view_; }

  ShortcutCategory category() { return category_; }

  const KeyboardShortcutItem* shortcut_item() { return shortcut_item_; }

 private:
  ShortcutCategory category_;

  // View of the text describing what action the shortcut performs.
  views::StyledLabel* description_label_view_;

  // View of the text listing the keys making up the shortcut.
  views::StyledLabel* shortcut_label_view_;

  // Not owned. Pointer to the keyboard shortcut item.
  const KeyboardShortcutItem* shortcut_item_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutItemView);
};

}  // namespace keyboard_shortcut_viewer

#endif  // UI_CHROMEOS_KSV_VIEWS_KEYBOARD_SHORTCUT_ITEM_VIEW_H_
