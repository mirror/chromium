// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/text.h"

#include "base/memory/ptr_util.h"
#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/elements/text_texture.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/render_text.h"

namespace vr {

namespace {
constexpr float kCursorWidthRatio = 0.07f;
}

Text::Text(int maximum_width_pixels, float font_height_meters)
    : TexturedElement(maximum_width_pixels),
      texture_(base::MakeUnique<TextTexture>(font_height_meters)) {}
Text::~Text() {}

void Text::SetText(const base::string16& text) {
  texture_->SetText(text);
}

void Text::SetColor(SkColor color) {
  texture_->SetColor(color);
}

void Text::SetTextAlignment(UiTexture::TextAlignment alignment) {
  texture_->SetAlignment(alignment);
}

void Text::SetMultiLine(bool multiline) {
  texture_->SetMultiLine(multiline);
}

void Text::SetCursorEnabled(bool enabled) {
  texture_->SetCursorEnabled(enabled);
}

void Text::SetCursorPosition(int position) {
  texture_->SetCursorPosition(position);
}

gfx::RectF Text::GetCursorBounds() {
  // Note that gfx:: cursor bounds always indicate a one-pixel width, so we
  // override the width here to be a percentage of height for the sake of
  // arbitrary texture sizes.
  gfx::Rect bounds = texture_->get_cursor_bounds();
  float scale = size().width() / texture_->GetDrawnSize().width();
  return gfx::RectF(
      bounds.CenterPoint().x() * scale, bounds.CenterPoint().y() * scale,
      bounds.height() * scale * kCursorWidthRatio, bounds.height() * scale);
}

void Text::OnSetSize(gfx::SizeF size) {
  texture_->SetTextWidth(size.width());
}

TextTexture* Text::GetTextureForTest() {
  return texture_.get();
}

UiTexture* Text::GetTexture() const {
  return texture_.get();
}

}  // namespace vr
