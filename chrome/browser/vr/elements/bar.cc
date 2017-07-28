// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/bar.h"

#include "base/memory/ptr_util.h"
#include "cc/base/math_util.h"
#include "chrome/browser/vr/elements/bar_texture.h"

namespace vr {

namespace {

bool ApproximatelyEqual(float lhs, float rhs, float tolerance) {
  DCHECK_LE(0, tolerance);
  return std::abs(rhs - lhs) <= tolerance;
}

}  // namespace

Bar::Bar() : TexturedElement(512), texture_(base::MakeUnique<BarTexture>()) {}

Bar::~Bar() = default;

void Bar::SetSize(float width, float height) {
  auto prev_size = size();
  TexturedElement::SetSize(width, height);
  // Currently, we resize textured elements based on the drawn size of the
  // texture. If we get called due to this resizing don't make the texture
  // dirty. Otherwise, rendering will fail.
  // TODO(tiborg): remove once frame update phases are established.
  float tolerance = 0.00001;
  if (ApproximatelyEqual(width, prev_size.width(), tolerance) &&
      ApproximatelyEqual(height, prev_size.height(), tolerance)) {
    return;
  }
  texture_->SetSize(width, height);
}

void Bar::SetColor(SkColor color) {
  texture_->SetColor(color);
}

UiTexture* Bar::GetTexture() const {
  return texture_.get();
}

}  // namespace vr
