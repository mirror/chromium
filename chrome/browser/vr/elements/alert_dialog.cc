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
                         base::string16 txt_message)
    : TexturedElement(preferred_width), height_meters_(height_meters) {
    texture_ = base::MakeUnique<AlertDialogTexture>(icon, txt_message);
}

void AlertDialog::UpdateElementSize() {
  gfx::SizeF drawn_size = GetTexture()->GetDrawnSize();
  float width = height_meters_ * drawn_size.width() / drawn_size.height();
  SetSize(width, height_meters_);
}
void AlertDialog::SetText(base::string16 new_text) {
   texture_->SetText(new_text);
}

AlertDialog::~AlertDialog() = default;

UiTexture* AlertDialog::GetTexture() const {
  return texture_.get();
}

}  // namespace vr
