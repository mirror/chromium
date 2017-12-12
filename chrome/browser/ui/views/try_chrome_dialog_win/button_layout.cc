// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/try_chrome_dialog_win/button_layout.h"

#include "base/logging.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

namespace {
constexpr int kPaddingBetweenButtons = 4;
}  // namespace

// static
ButtonLayout* ButtonLayout::CreateAndInstall(views::View* view,
                                             int view_width) {
  ButtonLayout* layout = new ButtonLayout(view_width);
  view->SetLayoutManager(layout);
  return layout;
}

ButtonLayout::~ButtonLayout() = default;

void ButtonLayout::Layout(views::View* host) {
  const gfx::Size& host_size = host->bounds().size();
  const gfx::Size max_child_size = GetMaxChildSize(host);
  const bool use_wide_buttons =
      UseWideButtons(host_size.width(), max_child_size.width());
  const int child_count = host->child_count();

  // The buttons are all equal-sized.
  gfx::Size button_size(0, max_child_size.height());
  if (use_wide_buttons)
    button_size.set_width(host_size.width());
  else
    button_size.set_width((host_size.width() - kPaddingBetweenButtons) / 2);

  if (!use_wide_buttons) {
    // The offset of the right-side narrow button.
    int right_x = button_size.width() + kPaddingBetweenButtons;
    // If there is only one narrow button, position it on the right.
    if (child_count == 1) {
      host->child_at(0)->SetBoundsRect({{right_x, 0}, button_size});
    } else {
      host->child_at(0)->SetBoundsRect({{0, 0}, button_size});
      host->child_at(1)->SetBoundsRect({{right_x, 0}, button_size});
    }
  } else {
    host->child_at(0)->SetBoundsRect({{0, 0}, button_size});
    if (child_count != 1) {
      int bottom_y = button_size.height() + kPaddingBetweenButtons;
      host->child_at(1)->SetBoundsRect({{0, bottom_y}, button_size});
    }
  }
}

gfx::Size ButtonLayout::GetPreferredSize(const views::View* host) const {
  const gfx::Size max_child_size = GetMaxChildSize(host);

  // If there's only one button or the widest of two is sufficiently narrow,
  // only one row is needed.
  if (host->child_count() == 1 ||
      !UseWideButtons(view_width_, max_child_size.width())) {
    return {view_width_, max_child_size.height()};
  }

  // Two rows of equal height with padding between them.
  return {view_width_, max_child_size.height() * 2 + kPaddingBetweenButtons};
}

ButtonLayout::ButtonLayout(int view_width) : view_width_(view_width) {}

gfx::Size ButtonLayout::GetMaxChildSize(const views::View* host) const {
  gfx::Size max_child_size;
  const int child_count = host->child_count();
  DCHECK_GE(child_count, 1);
  DCHECK_LE(child_count, 2);
  for (int i = 0; i < child_count; ++i) {
    const views::View* child = host->child_at(i);
    max_child_size.SetToMax(child->GetPreferredSize());
  }
  return max_child_size;
}

// static
bool ButtonLayout::UseWideButtons(int host_width, int max_child_width) {
  return max_child_width > (host_width - kPaddingBetweenButtons) / 2;
}
