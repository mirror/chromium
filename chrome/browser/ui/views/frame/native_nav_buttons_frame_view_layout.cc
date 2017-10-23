// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/native_nav_buttons_frame_view_layout.h"

#include "base/command_line.h"
#include "chrome/browser/ui/views/nav_button_provider.h"
#include "chrome/common/chrome_switches.h"

NativeNavButtonsFrameViewLayout::NativeNavButtonsFrameViewLayout(
    OpaqueBrowserFrameViewLayoutDelegate* delegate)
    : OpaqueBrowserFrameViewLayout(delegate) {}

int NativeNavButtonsFrameViewLayout::CaptionButtonY(
    chrome::FrameButtonDisplayType button_id,
    bool restored) const {
  auto* button_provider = delegate_->GetNavButtonProvider();
  gfx::Insets insets = button_provider->GetNavButtonMargin(button_id);
  return insets.top();
}

int NativeNavButtonsFrameViewLayout::TopAreaPadding() const {
  auto* button_provider = delegate_->GetNavButtonProvider();
  return button_provider->GetTopAreaSpacing().left();
}

int NativeNavButtonsFrameViewLayout::GetWindowCaptionSpacing(
    views::FrameButton button_id,
    bool leading_spacing,
    bool is_leading_button) const {
  auto* button_provider = delegate_->GetNavButtonProvider();
  gfx::Insets insets =
      button_provider->GetNavButtonMargin(GetButtonDisplayType(button_id));
  if (!leading_spacing)
    return insets.right();
  int spacing = insets.left();
  if (!is_leading_button)
    spacing += button_provider->GetInterNavButtonSpacing();
  return spacing;
}

void NativeNavButtonsFrameViewLayout::LayoutNewStyleAvatar(views::View* host) {
  if (!new_avatar_button_)
    return;

  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableNativeAvatarButton)) {
    OpaqueBrowserFrameViewLayout::LayoutNewStyleAvatar(host);
    return;
  }
  auto* button_provider = delegate_->GetNavButtonProvider();
  gfx::Size button_size;
  gfx::Insets button_spacing;
  button_provider->CalculateCaptionButtonLayout(
      new_avatar_button_->GetPreferredSize(), delegate_->GetTopAreaHeight(),
      &button_size, &button_spacing);
  int inter_button_spacing = button_provider->GetInterNavButtonSpacing();

  int total_width =
      button_size.width() + button_spacing.right() + inter_button_spacing;

  int button_x = host->width() - trailing_button_start_ - total_width;
  int button_y = button_spacing.top();

  minimum_size_for_buttons_ += total_width;
  trailing_button_start_ += total_width;

  new_avatar_button_->SetBounds(button_x, button_y, button_size.width(),
                                button_size.height());
}

bool NativeNavButtonsFrameViewLayout::ShouldDrawImageMirrored(
    views::ImageButton* button,
    ButtonAlignment alignment) const {
  return false;
}
