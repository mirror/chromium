// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/views/keyboard_shortcut_item_view.h"

#include <vector>

#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/ksv/keyboard_shortcut_viewer_metadata.h"
#include "ui/views/controls/styled_label.h"

namespace keyboard_shortcut_viewer {

namespace {

// TODO(wutao): to be decided by UX specs.
constexpr float kShortcutViewRatio = 0.618f;

}  // namespace

KeyboardShortcutItemView::KeyboardShortcutItemView(
    const KeyboardShortcutItem& item) {
  description_label_view_ = new views::StyledLabel(
      l10n_util::GetStringUTF16(item.description_message_id), nullptr);
  AddChildView(description_label_view_);

  std::vector<size_t> offsets;
  std::vector<base::string16> replacement_strings;
  bool shift_down = false;
  for (size_t i = 0; i < item.shortcut_key_codes.size(); ++i) {
    ui::KeyboardCode keycode = item.shortcut_key_codes[i];
    if (keycode == ui::VKEY_SHIFT)
      shift_down = true;
    replacement_strings.push_back(
        GetStringForKeyboardCode(item.shortcut_key_codes[i], shift_down));
  }

  if (replacement_strings.empty()) {
    shortcut_label_view_ = new views::StyledLabel(
        l10n_util::GetStringUTF16(item.shortcut_message_id), nullptr);
  } else {
    shortcut_label_view_ = new views::StyledLabel(
        l10n_util::GetStringFUTF16(item.shortcut_message_id,
                                   replacement_strings, &offsets),
        nullptr);
  }
  DCHECK_EQ(replacement_strings.size(), offsets.size());
  for (size_t i = 0; i < offsets.size(); ++i) {
    views::StyledLabel::RangeStyleInfo style_info;
    // TODO(wutao): add rounded bubble views to highlight shortcut keys when
    // views::StyledLabel supports custom views.
    // TODO(wutao): add icons for keys.
    // TODO(wutao): finalize the sytles with UX specs.
    style_info.override_color = SK_ColorBLUE;
    style_info.disable_line_wrapping = true;
    shortcut_label_view_->AddStyleRange(
        gfx::Range(offsets[i], offsets[i] + replacement_strings[i].length()),
        style_info);
  }
  AddChildView(shortcut_label_view_);
}

int KeyboardShortcutItemView::GetHeightForWidth(int w) const {
  int width_shortcut_view = w * kShortcutViewRatio;
  int width_description_view = w - width_shortcut_view;
  int height_shortcut_view =
      shortcut_label_view_->GetHeightForWidth(width_shortcut_view);
  int height_description_view =
      description_label_view_->GetHeightForWidth(width_description_view);
  return std::max(height_description_view, height_shortcut_view);
}

void KeyboardShortcutItemView::Layout() {
  gfx::Rect rect(GetContentsBounds());
  if (rect.IsEmpty())
    return;

  int width_shortcut_view = rect.width() * kShortcutViewRatio;
  int width_description_view = rect.width() - width_shortcut_view;
  int height = GetHeightForWidth(rect.width());

  // TODO(wutao): addjust two views' bounds based UX specs.
  gfx::Rect description_label_rect;
  description_label_rect.set_width(width_description_view);
  description_label_rect.set_height(height);
  description_label_view_->SetBoundsRect(description_label_rect);

  gfx::Rect shortcut_label_rect;
  shortcut_label_rect.set_width(width_shortcut_view);
  shortcut_label_rect.set_height(height);
  shortcut_label_rect.set_x(width_description_view);
  shortcut_label_view_->SetBoundsRect(shortcut_label_rect);
}

}  // namespace keyboard_shortcut_viewer
