// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/tooltip.h"

#include "base/memory/ptr_util.h"
#include "cc/base/math_util.h"
#include "chrome/browser/vr/elements/tooltip_texture.h"

namespace vr {

Tooltip::Tooltip()
    : TexturedElement(512), texture_(base::MakeUnique<TooltipTexture>()) {}

Tooltip::~Tooltip() = default;

void Tooltip::SetSize(float width, float height) {
  auto prev_size = size();
  TexturedElement::SetSize(width, height);
  // Currently, we resize textured elements based on the drawn size of the
  // texture. If we get called due to this resizing don't make the texture
  // dirty. Otherwise, rendering will fail.
  // TODO(tiborg): remove once frame update phases are established.
  float tolerance = 0.00001;
  if (cc::MathUtil::ApproximatelyEqual(width, prev_size.width(), tolerance) &&
      cc::MathUtil::ApproximatelyEqual(height, prev_size.height(), tolerance)) {
    return;
  }
  texture_->SetSize(width, height);
}

void Tooltip::SetText(const base::string16& text) {
  texture_->SetText(text);
}

void Tooltip::SetBackgroundColor(SkColor color) {
  texture_->SetBackgroundColor(color);
}

void Tooltip::SetTextColor(SkColor color) {
  texture_->SetTextColor(color);
}

UiTexture* Tooltip::GetTexture() const {
  return texture_.get();
}

}  // namespace vr
