// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/alert_dialog.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/elements/alert_dialog_texture.h"

namespace vr {

AlertDialog::AlertDialog(int preferred_width,
                         float height_meters,
                         const gfx::VectorIcon& icon,
                         base::string16 txt_message,
                         base::string16 second_message,
                         base::string16 buttons,
                         base::string16 buttons2)
    : TexturedElement(preferred_width), height_meters_(height_meters) {
  texture_ = base::MakeUnique<AlertDialogTexture>(
      icon, txt_message, second_message, buttons, buttons2);
}

void AlertDialog::UpdateElementSize() {
  gfx::SizeF drawn_size = GetTexture()->GetDrawnSize();
  float width = height_meters_ * drawn_size.width() / drawn_size.height();
  SetSize(width, height_meters_);
}

void AlertDialog::SetContent(long icon,
                             base::string16 title_text,
                             base::string16 toggle_text,
                             int b_positive,
                             base::string16 b_positive_text,
                             int b_negative,
                             base::string16 b_negative_text) {
  texture_->SetContent(icon, title_text, toggle_text, b_positive,
                       b_positive_text, b_negative, b_negative_text);
}

AlertDialog::~AlertDialog() = default;

UiTexture* AlertDialog::GetTexture() const {
  return texture_.get();
}

void AlertDialog::OnHoverEnter(const gfx::PointF& position) {
  OnStateUpdated(position);
}

void AlertDialog::OnHoverLeave() {
  OnStateUpdated(gfx::PointF(std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max()));
}

void AlertDialog::OnMove(const gfx::PointF& position) {
  OnStateUpdated(position);
}

void AlertDialog::OnStateUpdated(const gfx::PointF& position) {
  texture_->UpdateHoverState(position);
  UpdateTexture();
}

}  // namespace vr
